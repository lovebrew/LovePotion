#include "common/runtime.h"
#include "modules/font/fontmodule.h"

#include "common/data.h"

using namespace love;

class DefaultFontData : public love::Data
{
    public:
        DefaultFontData()
        { plGetSharedFontByType(&this->fontData, PlSharedFontType_Standard); }

        Data * Clone()   const override { return new DefaultFontData();  }
        void * GetData() const override { return this->fontData.address; }
        size_t GetSize() const override { return this->fontData.size;    }

    private:
        PlFontData fontData;
};

FontModule::FontModule()
{
    if (FT_Init_FreeType(&this->library))
        throw love::Exception("TrueTypeFont Loading error: FT_Init_FreeType failed");
}

FontModule::~FontModule()
{
    FT_Done_FreeType(this->library);
}

love::Rasterizer * FontModule::NewRasterizer(love::FileData * data)
{
    if (love::TrueTypeRasterizer::Accepts(this->library, data))
        return this->NewTrueTypeRasterizer(data, 12, love::TrueTypeRasterizer::HINTING_NORMAL);

    throw love::Exception("Invalid font file: %s", data->GetFilename().c_str());
}

love::Rasterizer * FontModule::NewTrueTypeRasterizer(love::Data * data, int size, love::TrueTypeRasterizer::Hinting hinting)
{
    return new love::TrueTypeRasterizer(this->library, data, size, hinting);
}

love::Rasterizer * FontModule::NewTrueTypeRasterizer(int size, TrueTypeRasterizer::Hinting hinting)
{
    love::StrongReference<DefaultFontData> data(new DefaultFontData, Acquire::NORETAIN);
    return NewTrueTypeRasterizer(data.Get(), size, hinting);
}