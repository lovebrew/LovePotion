#include <objects/texture_ext.hpp>

#include <utilities/driver/renderer_ext.hpp>

#include <gx2/utils.h>

#include <malloc.h>

using namespace love;

static void createTextureObject(GX2Texture*& texture, PixelFormat format, int width, int height)
{
    texture = new GX2Texture();

    if (!texture)
        throw love::Exception("Failed to create GX2Texture.");

    texture->surface.use    = GX2_SURFACE_USE_TEXTURE;
    texture->surface.dim    = GX2_SURFACE_DIM_TEXTURE_2D;
    texture->surface.width  = width;
    texture->surface.height = height;

    texture->surface.depth     = 1;
    texture->surface.mipLevels = 1;

    std::optional<GX2SurfaceFormat> gxFormat;
    if (!(gxFormat = Renderer<Console::CAFE>::pixelFormats.Find(format)))
        throw love::Exception("Invalid pixel format.");

    texture->surface.format   = *gxFormat;
    texture->surface.aa       = GX2_AA_MODE1X;
    texture->surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
    texture->viewFirstMip     = 0;
    texture->viewNumMips      = 1;
    texture->viewFirstSlice   = 0;
    texture->viewNumSlices    = 1;
    texture->compMap = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);

    GX2CalcSurfaceSizeAndAlignment(&texture->surface);
    GX2InitTextureRegs(texture);

    texture->surface.image = memalign(texture->surface.alignment, texture->surface.imageSize);

    if (!texture->surface.image)
        throw love::Exception("Failed to create GX2Surface.");

    std::memset(texture->surface.image, 0, texture->surface.imageSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture->surface.image,
                  texture->surface.imageSize);
}

Texture<Console::CAFE>::Texture(const Graphics<Console::CAFE>* graphics, const Settings& settings,
                                const Slices* data) :
    Texture<Console::ALL>(settings, data),
    framebuffer {},
    texture {},
    sampler {}
{
    this->format = graphics->GetSizedFormat(format, this->renderTarget, this->readable);

    if (this->mipmapMode == MIPMAPS_AUTO && this->IsCompressed())
        this->mipmapMode = MIPMAPS_MANUAL;

    if (this->mipmapMode != MIPMAPS_NONE)
        this->mipmapCount =
            Texture<>::GetTotalMipmapCount(this->pixelWidth, this->pixelHeight, this->depth);

    bool invalidDimensions = this->pixelWidth <= 0 || this->pixelHeight <= 0;
    if (invalidDimensions || this->layers <= 0 || this->depth <= 0)
        throw love::Exception("Texture dimensions must be greater than zero.");

    if (this->textureType != TEXTURE_2D && this->requestedMSAA > 1)
        throw love::Exception("MSAA is only supported for 2D textures.");

    if (!this->renderTarget && this->requestedMSAA > 1)
        throw love::Exception("MSAA is only supported with render target textures.");

    bool isDepthStencilFormat = love::IsPixelFormatDepthStencil(this->format);
    if (this->readable && isDepthStencilFormat && settings.msaa > 1)
        throw love::Exception("Readable depth/stencil textures with MSAA are not supported.");

    if ((!this->readable || settings.msaa > 1) && this->mipmapMode != MIPMAPS_NONE)
        throw love::Exception("Non-readable and MSAA textures cannot have mipmaps.");

    if (!this->readable && this->textureType != TEXTURE_2D)
        throw love::Exception("Non-readable pixel formats are only supported for 2D textures.");

    if (this->IsCompressed() && this->renderTarget)
        throw love::Exception("Compressed textures cannot be render targets.");

    this->state = graphics->GetDefaultSamplerState();
    if (this->GetMipmapCount() == 1)
        this->state.mipmapFilter = SamplerState::MIPMAP_FILTER_NONE;

    Quad::Viewport view { 0, 0, (double)this->width, (double)this->height };
    this->quad.Set(new Quad(view, this->width, this->height), Acquire::NORETAIN);

    ++textureCount;

    if (data != nullptr)
        slices = *data;

    this->LoadVolatile();

    slices.Clear();
}

Texture<Console::CAFE>::~Texture()
{
    this->UnloadVolatile();
}

bool Texture<Console::CAFE>::LoadVolatile()
{
    if (this->IsReadable())
        this->CreateTexture();

    int64_t memorySize = 0;

    for (int mipmap = 0; mipmap < this->GetMipmapCount(); mipmap++)
    {
        const auto width  = this->GetPixelWidth(mipmap);
        const auto height = this->GetPixelHeight(mipmap);

        const auto faceCount = this->textureType == TEXTURE_CUBE ? 6 : 1;
        const auto slices    = this->GetDepth(mipmap) * this->layers * faceCount;

        memorySize += love::GetPixelFormatSliceSize(this->format, width, height) * slices;
    }

    this->SetGraphicsMemorySize(memorySize);

    return true;
}

void Texture<Console::CAFE>::CreateTexture()
{
    Texture<Console::ALL>::CreateTexture();
    bool hasData = this->slices.Get(0, 0) != nullptr;

    int _width  = this->pixelWidth;
    int _height = this->pixelHeight;

    if (this->IsRenderTarget())
    {
        bool clear = !hasData;
        // createFramebufferObject(this->framebuffer, this->image.tex, _width, _height);
    }
    else
    {
        Rect rectangle { 0, 0, _width, _height };

        createTextureObject(this->texture, this->format, _width, _height);
        const auto copySize = love::GetPixelFormatSliceSize(this->format, _width, _height);

        this->ReplacePixels(this->slices.Get(0, 0), 0, 0, 0, 0, false);
    }

    this->SetSamplerState(this->state);
}

void Texture<Console::CAFE>::UnloadVolatile()
{
    if (this->texture)
        delete this->texture;
}

void Texture<Console::CAFE>::ReplacePixels(ImageData<Console::CAFE>* data, int slice, int mipmap,
                                           int x, int y, bool reloadMipmaps)
{
    if (!this->IsReadable())
        throw love::Exception("replacePixels can only be called on readable Textures.");

    if (this->GetMSAA() > 1)
        throw love::Exception("replacePixels cannot be called on an MSAA Texture.");

    auto* graphics = Module::GetInstance<Graphics<Console::CAFE>>(Module::M_GRAPHICS);

    if (graphics != nullptr && graphics->IsRenderTargetActive(this))
        throw love::Exception(
            "replacePixels cannot be called on this Texture while it's an active render target.");

    if (this->texture == nullptr)
        return;

    if (data->GetFormat() != this->GetPixelFormat())
        throw love::Exception("Pixel formats must match.");

    if (mipmap < 0 || mipmap >= this->GetMipmapCount())
        throw love::Exception("Invalid texture mipmap index %d.", mipmap + 1);

    const bool isCubeType   = this->textureType == TEXTURE_CUBE;
    const bool isVolumeType = this->textureType == TEXTURE_VOLUME;
    const bool isArrayType  = this->textureType == TEXTURE_2D_ARRAY;

    if (slice < 0 || (isCubeType && slice >= 6) ||
        (isVolumeType && slice >= this->GetDepth(mipmap)) ||
        (isArrayType && slice >= this->GetLayerCount()))
    {
        throw love::Exception("Invalid texture slice index %d", slice + 1);
    }

    Rect rectangle = { x, y, data->GetWidth(), data->GetHeight() };

    int mipWidth  = this->GetPixelWidth(mipmap);
    int mipHeight = this->GetPixelWidth(mipmap);

    if (rectangle.x < 0 || rectangle.y < 0 || rectangle.w <= 0 || rectangle.h <= 0 ||
        (rectangle.x + rectangle.w) > mipWidth || (rectangle.y + rectangle.h) > mipHeight)
    {
        throw love::Exception(
            "Invalid rectangle dimensions (x = %d, y = %d, w = %d, h = %d) for %dx%d Texture.",
            rectangle.x, rectangle.y, rectangle.w, rectangle.h, mipWidth, mipHeight);
    }

    this->ReplacePixels(data->GetData(), data->GetSize(), 0, 0, rectangle, false);
}

void Texture<Console::CAFE>::ReplacePixels(const void* data, size_t size, int slice, int mipmap,
                                           const Rect& rect, bool reloadMipmaps)
{
    if (!this->IsReadable() || this->GetMSAA() > 1)
        return;

    auto* graphics = Module::GetInstance<Graphics<Console::CAFE>>(Module::M_GRAPHICS);

    if (graphics != nullptr && graphics->IsRenderTargetActive(this))
        return;

    const auto pitch     = this->texture->surface.pitch;
    uint8_t* destination = (uint8_t*)this->texture->surface.image;

    /* copy by row */
    for (uint32_t y = 0; y < std::min(rect.h, this->height - rect.y); ++y)
    {
        const auto row = (y * std::min(rect.w, this->width - rect.x) * 4);
        std::memcpy(destination + (y * pitch * 4), data + row,
                    std::min(rect.w, this->width - rect.x) * 4);
    }

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, this->texture->surface.image,
                  this->texture->surface.imageSize);
}

void Texture<Console::CAFE>::SetSamplerState(const SamplerState& state)
{
    Texture<>::SetSamplerState(state);

    this->state.magFilter = this->state.minFilter = SamplerState::FILTER_NEAREST;

    if (this->state.mipmapFilter == SamplerState::MIPMAP_FILTER_LINEAR)
        this->state.mipmapFilter = SamplerState::MIPMAP_FILTER_NEAREST;

    Renderer<Console::CAFE>::Instance().SetSamplerState(this, this->state);
}
