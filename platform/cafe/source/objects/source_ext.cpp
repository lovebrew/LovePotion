#include <objects/source_ext.hpp>
#include <utilities/pool/sources.hpp>

#include <coreinit/cache.h>

#include <algorithm>
#include <utilities/log/logfile.h>

using namespace love;

using DSP = love::DSP<Console::CAFE>;

template<>
Source<Console::CAFE>::DataBuffer::DataBuffer(const void* data, size_t size) : size(size)
{
    this->buffer = (int16_t*)malloc(size);
    std::memcpy(this->buffer, (int16_t*)data, size);
}

template<>
Source<Console::CAFE>::DataBuffer::~DataBuffer()
{
    free(this->buffer);
}

Source<Console::CAFE>::Source(AudioPool* pool, SoundData* soundData) :
    Source<>(TYPE_STATIC),
    pool(pool)
{
    this->sampleRate    = soundData->GetSampleRate();
    this->channels      = soundData->GetChannelCount();
    this->bitDepth      = soundData->GetBitDepth();
    this->samplesOffset = 0;
    this->bufferCount   = 1;

    std::memset(this->buffers, 0, sizeof(this->buffers));
    this->staticBuffer = std::make_shared<DataBuffer>(soundData->GetData(), soundData->GetSize());
}

Source<Console::CAFE>::Source(AudioPool* pool, Decoder* decoder) : Source<>(TYPE_STREAM), pool(pool)
{
    this->decoder       = decoder;
    this->sampleRate    = decoder->GetSampleRate();
    this->channels      = decoder->GetChannelCount();
    this->bitDepth      = decoder->GetBitDepth();
    this->bufferCount   = MAX_BUFFERS;
    this->samplesOffset = 0;
}

Source<Console::CAFE>::Source(AudioPool* pool, int sampleRate, int bitDepth, int channels,
                              int buffers) :
    Source<>(TYPE_QUEUE),
    pool(pool)
{
    this->sampleRate    = sampleRate;
    this->channels      = channels;
    this->bitDepth      = bitDepth;
    this->bufferCount   = buffers;
    this->samplesOffset = 0;

    if (buffers < 1 || buffers > Source::MAX_BUFFERS)
        buffers = MAX_BUFFERS;
}

Source<Console::CAFE>::Source(const Source& other) : Source<>(other.sourceType), pool(other.pool)
{
    this->staticBuffer  = other.staticBuffer;
    this->decoder       = nullptr;
    this->sampleRate    = other.sampleRate;
    this->channels      = other.channels;
    this->bitDepth      = other.bitDepth;
    this->bufferCount   = other.bufferCount;
    this->samplesOffset = other.samplesOffset;

    if (this->sourceType == TYPE_STREAM)
    {
        if (other.decoder.Get())
            this->decoder.Set(other.decoder->Clone(), Acquire::NORETAIN);
    }
}

Source<Console::CAFE>::~Source()
{
    this->Stop();
}

Source<Console::CAFE>* Source<Console::CAFE>::Clone()
{
    return new Source(*this);
}

bool Source<Console::CAFE>::Play()
{
    uint8_t wasPlaying = false;

    {
        auto lock = this->pool->Lock();
        if (!this->pool->AssignSource(this, this->channel, wasPlaying))
            return this->valid = false;
    }

    if (!wasPlaying)
    {
        if (!(this->valid = this->PlayAtomic(this->buffers[0])))
            return false;

        this->ResumeAtomic();

        {
            auto lock = this->pool->Lock();
            this->pool->AddSource(this, this->channel);
        }

        return this->valid;
    }

    this->ResumeAtomic();

    return this->valid = true;
}

void Source<Console::CAFE>::Reset()
{}

void Source<Console::CAFE>::Stop()
{
    if (!this->valid)
        return;

    auto lock = this->pool->Lock();
    this->pool->ReleaseSource(this);
}

void Source<Console::CAFE>::Pause()
{
    auto lock = this->pool->Lock();

    if (this->pool->IsPlaying(this))
        this->PauseAtomic();
}

bool Source<Console::CAFE>::IsPlaying() const
{
    return this->valid && !::DSP::Instance().IsChannelPaused(this->channel);
}

bool Source<Console::CAFE>::IsFinished() const
{
    if (!this->valid)
        return false;

    if (this->sourceType == TYPE_STREAM && (this->IsLooping() || !this->decoder->IsFinished()))
        return false;

    return ::DSP::Instance().IsChannelPlaying(this->channel) == false;
}

bool Source<Console::CAFE>::Update()
{
    if (!this->valid)
        return false;

    switch (this->sourceType)
    {
        case TYPE_STATIC:
            return !this->IsFinished();
        case TYPE_STREAM:
        {
            if (this->IsFinished())
                return false;

            bool other = !this->current;

            if (::DSP::Instance().IsChannelPlaying(this->channel))
            {
                int decoded = this->StreamAtomic(this->buffers[other], this->decoder.Get());

                if (decoded == 0)
                    return false;

                ::DSP::Instance().ChannelAddBuffer(this->channel, this->buffers[other], false);
                this->samplesOffset += ::DSP::Instance().ChannelGetSampleOffset(this->channel);

                this->current = !this->current;
            }

            return true;
        }
        case TYPE_QUEUE:
            break;
        default:
            break;
    }

    return false;
}

void Source<Console::CAFE>::SetVolume(float volume)
{
    if (volume < this->GetMinVolume() || volume > this->GetMaxVolume())
        return;

    if (this->valid)
        ::DSP::Instance().ChannelSetVolume(this->channel, volume);

    this->volume = volume;
}

float Source<Console::CAFE>::GetVolume() const
{
    if (this->valid)
        return ::DSP::Instance().ChannelGetVolume(this->channel);

    return this->volume;
}

/* todo */
void Source<Console::CAFE>::Seek(double offset, Unit unit)
{
    // auto lock = this->pool->Lock();

    int offsetSamples    = 0;
    double offsetSeconds = 0.0f;

    switch (unit)
    {
        case UNIT_SAMPLES:
        {
            offsetSamples = (int)offset;
            offsetSeconds = offset / ((double)this->sampleRate / this->channels);
            break;
        }
        case UNIT_SECONDS:
        default:
        {
            offsetSeconds = offset;
            offsetSamples = (int)(offset * sampleRate * this->channels);
        }
    }

    bool wasPlaying = this->IsPlaying();

    switch (this->sourceType)
    {
        case TYPE_STATIC:
        {
            if (this->valid)
                this->Stop();

            this->samplesOffset = offsetSamples;

            if (wasPlaying)
                this->Play();

            break;
        }
        case TYPE_STREAM:
        {
            if (this->valid)
                this->Stop();

            this->decoder->Seek(offsetSeconds);

            if (wasPlaying)
                this->Play();

            break;
        }
        case TYPE_QUEUE:
        {
            /* todo */
        }
        default:
            break;
    }

    if (wasPlaying && (this->sourceType == TYPE_STREAM && !this->IsPlaying()))
    {
        this->Stop();

        if (this->IsLooping())
            this->Play();

        return;
    }

    this->samplesOffset = offsetSamples;
}

/* todo */
double Source<Console::CAFE>::Tell(Unit unit)
{
    auto lock = this->pool->Lock();

    int offset = 0;

    if (this->valid)
    {
        if (this->sourceType == TYPE_STATIC)
            offset += ::DSP::Instance().ChannelGetSampleOffset(this->channel);
        else
            offset = this->samplesOffset;
    }

    if (unit == UNIT_SECONDS)
        return offset / (double)sampleRate / this->channels;

    return offset;
}

double Source<Console::CAFE>::GetDuration(Unit unit)
{
    auto lock = this->pool->Lock();

    switch (this->sourceType)
    {
        case TYPE_STATIC:
        {
            size_t size    = this->staticBuffer->GetSize();
            size_t samples = (size / this->channels) / (this->bitDepth / 8);

            if (unit == UNIT_SAMPLES)
                return (double)samples;

            return (double)samples / (double)sampleRate;
        }
        case TYPE_STREAM:
        {
            /* vorbisidec 1.2.1 uses ms, not sec, convert */
            double seconds = this->decoder->GetDuration() / 1000.0;

            if (unit == UNIT_SECONDS)
                return seconds;

            return seconds * decoder->GetSampleRate();
        }
        case TYPE_QUEUE:
        {
            /* todo */
            break;
        }
        default:
            return 0.0;
    }

    return 0.0;
}

void Source<Console::CAFE>::SetLooping(bool loop)
{
    if (this->sourceType == TYPE_QUEUE)
        throw QueueLoopingException();

    this->looping = loop;
}

/* todo */
bool Source<Console::CAFE>::Queue(void* data, size_t length, int sampleRate, int bitDepth,
                                  int channels)
{
    if (this->sourceType != TYPE_QUEUE)
        throw QueueTypeMismatchException();

    if (sampleRate != this->sampleRate || bitDepth != this->bitDepth || channels != this->channels)
        throw QueueFormatMismatchException();

    if (length % (bitDepth / 8 * channels) != 0)
        throw QueueMalformedLengthException(bitDepth / 8 * channels);

    if (length == 0)
        return true;

    return true;
}

int Source<Console::CAFE>::GetFreeBufferCount() const
{
    if (this->sourceType == TYPE_STATIC)
        return 0;

    size_t count = 0;
    // for (auto& buffer : this->buffers)
    //     count += (buffer->state == ::DSP::STATE_FINISHED) ? 1 : 0;

    return count;
}

void Source<Console::CAFE>::PrepareAtomic()
{
    ::DSP::Instance().ChannelReset(this->channel);

    switch (this->sourceType)
    {
        case TYPE_STATIC:
        {
            const auto rawSize = this->staticBuffer->GetSize() - this->samplesOffset;

            // clang-format off
            this->buffers[0] = Mix_QuickLoad_RAW((uint8_t*)this->staticBuffer->GetBuffer() + (size_t)this->samplesOffset, rawSize);
            // clang-format on

            break;
        }
        case TYPE_STREAM:
        {
            if (this->StreamAtomic(this->buffers[0], this->decoder.Get()) == 0)
                break;

            if (this->decoder->IsFinished())
                break;

            break;
        }
        case TYPE_QUEUE:
            break; /* todo */
        default:
            break;
    }
}

int Source<Console::CAFE>::StreamAtomic(Mix_Chunk* buffer, Decoder* decoder)
{
    int decoded = std::max(decoder->Decode(), 0);

    if (decoded > 0)
        buffer = Mix_QuickLoad_RAW((uint8_t*)decoder->GetBuffer(), decoded);

    if (decoder->IsFinished() && this->IsLooping())
        decoder->Rewind();

    return decoded;
}

/* todo */
void Source<Console::CAFE>::TeardownAtomic()
{
    ::DSP::Instance().ChannelStop(this->channel);

    switch (this->sourceType)
    {
        case TYPE_STATIC:
            break;
        case TYPE_STREAM:
        {
            this->decoder->Rewind();

            break;
        }
        case TYPE_QUEUE:
            break; /* todo */
        default:
            break;
    }

    this->valid         = false;
    this->samplesOffset = 0;
}

bool Source<Console::CAFE>::PlayAtomic(Mix_Chunk* buffer)
{
    this->PrepareAtomic();

    bool looping = (this->sourceType == TYPE_STREAM) ? false : this->looping;
    if (!(this->valid = ::DSP::Instance().ChannelAddBuffer(this->channel, buffer, looping)))
        return false;

    if (this->sourceType != TYPE_STREAM)
        this->samplesOffset = 0;

    if (this->sourceType == TYPE_STREAM)
        this->valid = true;

    return true;
}

void Source<Console::CAFE>::StopAtomic()
{
    if (!this->valid)
        return;

    this->TeardownAtomic();
}

void Source<Console::CAFE>::PauseAtomic()
{
    if (this->valid)
        ::DSP::Instance().ChannelPause(this->channel);
}

void Source<Console::CAFE>::ResumeAtomic()
{
    if (this->valid)
        ::DSP::Instance().ChannelPause(this->channel, false);
}

bool Source<Console::CAFE>::Play(const std::vector<Source*>& sources)
{
    if (sources.size() == 0)
        return true;

    auto* pool = ((Source*)sources[0])->pool;
    auto lock  = pool->Lock();

    std::vector<uint8_t> wasPlaying(sources.size());
    std::vector<size_t> channels(sources.size());

    for (size_t index = 0; index < sources.size(); index++)
    {
        if (!pool->AssignSource((Source*)sources[index], channels[index], wasPlaying[index]))
        {
            for (size_t j = 0; j < index; j++)
            {
                if (!wasPlaying[j])
                    pool->ReleaseSource((Source*)sources[index], false);
            }

            return false;
        }
    }

    std::vector<Source*> toPlay;
    toPlay.reserve(sources.size());

    for (size_t index = 0; index < sources.size(); index++)
    {
        if (wasPlaying[index] && sources[index]->IsPlaying())
            continue;

        if (!wasPlaying[index])
        {
            auto* source    = (Source*)sources[index];
            source->channel = channels[index];

            source->PrepareAtomic();
        }

        toPlay.push_back(sources[index]);
    }

    for (auto& _source : toPlay)
    {
        auto* source = (Source*)_source;

        if (source->sourceType != TYPE_STREAM)
            source->samplesOffset = 0;

        if (!(_source->valid = _source->Play()))
            return false;

        pool->AddSource(_source, source->channel);
    }

    return true;
}

void Source<Console::CAFE>::Stop(const std::vector<Source*>& sources)
{
    if (sources.size() == 0)
        return;

    auto* pool = ((Source*)sources[0])->pool;
    auto lock  = pool->Lock();

    std::vector<Source*> toStop;
    toStop.reserve(sources.size());

    for (auto& _source : sources)
    {
        auto* source = (Source*)_source;

        if (source->valid)
            toStop.push_back(source);
    }

    for (auto& _source : toStop)
    {
        auto* source = (Source*)_source;

        if (source->valid)
            source->TeardownAtomic();

        pool->ReleaseSource(source, false);
    }
}

void Source<Console::CAFE>::Pause(const std::vector<Source*>& sources)
{
    if (sources.size() == 0)
        return;

    auto lock = ((Source*)sources[0])->pool->Lock();

    for (auto& _source : sources)
    {
        auto* source = (Source*)_source;

        if (source->valid)
            source->PauseAtomic();
    }
}

std::vector<Source<Console::CAFE>*> Source<Console::CAFE>::Pause(AudioPool* pool)
{
    std::vector<Source*> sources;

    {
        auto lock = pool->Lock();
        sources   = pool->GetPlayingSources();

        auto end = std::remove_if(sources.begin(), sources.end(),
                                  [](Source* source) { return !source->IsPlaying(); });

        sources.erase(end, sources.end());
    }

    Source::Pause(sources);

    return sources;
}

void Source<Console::CAFE>::Stop(AudioPool* pool)
{
    std::vector<Source*> sources;

    {
        auto lock = pool->Lock();
        sources   = pool->GetPlayingSources();
    }

    Source::Stop(sources);
}

int Source<Console::CAFE>::GetChannelCount() const
{
    return this->channels;
}
