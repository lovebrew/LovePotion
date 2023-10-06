#include <common/exception.hpp>

#include <utilities/driver/dsp_ext.hpp>

using namespace love;

static void audioCallback(void* data)
{
    auto event = (LightEvent*)data;
    LightEvent_Signal(event);
}

void DSP<Console::CTR>::Initialize()
{
    if (Result result; R_FAILED(result = ndspInit()))
    {
        if ((uint32_t)result == 0xD880A7FA)
            throw love::Exception("Failed to initialize ndsp (dspfirm.cdc not found)");

        throw love::Exception("Failed to initialize ndsp: %x", result);
    }

    this->initialized = true;

    LightEvent_Init(&this->event, RESET_ONESHOT);
    ndspSetCallback(audioCallback, &this->event);
}

DSP<Console::CTR>::~DSP()
{
    if (!this->initialized)
        return;

    ndspExit();
}

void DSP<Console::CTR>::Update()
{
    LightEvent_Wait(&this->event);
}

void DSP<Console::CTR>::SetMasterVolume(float volume)
{
    ndspSetMasterVol(volume);
}

float DSP<Console::CTR>::GetMasterVolume() const
{
    return ndspGetMasterVol();
}

bool DSP<Console::CTR>::ChannelReset(size_t id, int channels, int bitDepth, int sampleRate)
{
    ndspChnReset(id);

    uint8_t ndspFormat = 0;
    if ((ndspFormat = DSP::GetFormat(bitDepth, channels)) < 0)
        return false;

    std::optional<ndspInterpType> interpType;
    if (!(interpType = DSP::interpTypes.Find(channels)))
        return false;

    ndspChnSetFormat(id, ndspFormat);
    ndspChnSetRate(id, sampleRate);
    ndspChnSetInterp(id, *interpType);

    this->ChannelSetVolume(id, this->ChannelGetVolume(id));

    return true;
}

void DSP<Console::CTR>::ChannelSetVolume(size_t id, float volume)
{
    float mix[12] { 0.0f };
    mix[0] = mix[1] = volume;

    ndspChnSetMix(id, mix);
}

float DSP<Console::CTR>::ChannelGetVolume(size_t id) const
{
    float mix[12] { 0.0f };
    ndspChnGetMix(id, mix);

    return mix[0];
}

size_t DSP<Console::CTR>::ChannelGetSampleOffset(size_t id) const
{
    return ndspChnGetSamplePos(id);
}

bool DSP<Console::CTR>::ChannelAddBuffer(size_t id, ndspWaveBuf* buffer)
{
    ndspChnWaveBufAdd(id, buffer);

    return true;
}

void DSP<Console::CTR>::ChannelPause(size_t id, bool paused)
{
    ndspChnSetPaused(id, paused);
}

bool DSP<Console::CTR>::IsChannelPaused(size_t id) const
{
    return ndspChnIsPaused(id);
}

bool DSP<Console::CTR>::IsChannelPlaying(size_t id) const
{
    return ndspChnIsPlaying(id);
}

void DSP<Console::CTR>::ChannelStop(size_t id)
{
    ndspChnWaveBufClear(id);
}

int8_t DSP<Console::CTR>::GetFormat(int bitDepth, int channels)
{
    /* invalid bitDepth */
    if (bitDepth != 8 && bitDepth != 16)
        return -1;

    /* invalid channel count */
    if (channels < 0 || channels > 2)
        return -2;

    /* grab the encoding */
    uint8_t encoding = *DSP::audioFormats.Find(bitDepth);

    /* https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/ndsp/channel.h#L25 */
    return NDSP_CHANNELS(channels) | NDSP_ENCODING(encoding);
}
