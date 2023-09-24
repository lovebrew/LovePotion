#pragma once

#include <common/console.hpp>
#include <common/pixelformat.hpp>
#include <common/strongreference.hpp>

#include <objects/data/bytedata/bytedata.hpp>
#include <objects/imagedata/imagedatabase.hpp>

namespace love
{
    class CompressedSlice : public ImageDataBase
    {
      public:
        CompressedSlice(PixelFormat format, int width, int height, ByteData* memory, size_t offset,
                        size_t size);

        CompressedSlice(const CompressedSlice& slice);

        virtual ~CompressedSlice()
        {}

        CompressedSlice* Clone() const override;

        void* GetData() const override
        {
            return (uint8_t*)this->memory->GetData() + this->offset;
        }

        size_t GetSize() const override
        {
            return this->dataSize;
        }

        bool IsSRGB() const override
        {
            return this->sRGB;
        }

        size_t GetOffset() const
        {
            return this->offset;
        }

      private:
        StrongReference<ByteData> memory;

        size_t offset;
        size_t dataSize;
        bool sRGB;
    };
} // namespace love
