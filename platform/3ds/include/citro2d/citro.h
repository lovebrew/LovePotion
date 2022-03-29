#pragma once

#include "common/lmath.h"
#include "objects/canvas/canvas.h"

#include <citro2d.h>

#include "common/pixelformat.h"
#include "graphics/graphics.h"

class citro2d
{
  private:
    citro2d();

  public:
    struct GPUFilter
    {
        GPU_TEXTURE_FILTER_PARAM min;
        GPU_TEXTURE_FILTER_PARAM mag;

        GPU_PROCTEX_FILTER mipMap;
    };

    static citro2d& Instance();

    ~citro2d();

    void DestroyFramebuffers();

    void CreateFramebuffers();

    void BindFramebuffer(love::Canvas* canvas = nullptr);

    void ClearColor(const Colorf& color);

    void SetScissor(GPU_SCISSORMODE mode, const love::Rect& scissor, int screenWidth,
                    bool canvasActive);

    void SetStencil(GPU_TESTFUNC compare, int value);

    void Present();

    void SetTextureFilter(const love::Texture::Filter& filter);

    void SetTextureFilter(love::Texture* texture, const love::Texture::Filter& filter);

    void SetTextureWrap(const love::Texture::Wrap& wrap);

    void SetTextureWrap(love::Texture* texture, const love::Texture::Wrap& filter);

    template<typename T>
    void ModeChange(const T& func)
    {
        this->DestroyFramebuffers();
        func();
        this->CreateFramebuffers();
    }

    void SetWideMode(bool enable)
    {
        this->ModeChange([enable]() { gfxSetWide(enable); });
    }

    void Set3D(bool enable)
    {
        this->ModeChange([enable]() { gfxSet3D(enable); });
    }

    const bool Get3D() const
    {
        return gfxIs3D();
    }

    const bool GetWide() const
    {
        return gfxIsWide();
    }

    void DeferCallToEndOfFrame(std::function<void()>&& func)
    {
        this->deferredFunctions.emplace_back(std::move(func));
    }

    void SetBlendMode(GPU_BLENDEQUATION func, GPU_BLENDFACTOR srcColor, GPU_BLENDFACTOR srcAlpha,
                      GPU_BLENDFACTOR dstColor, GPU_BLENDFACTOR dstAlpha);

    void SetColorMask(const love::Graphics::ColorMask& mask);

    static GPU_TEXTURE_WRAP_PARAM GetCitroWrapMode(love::Texture::WrapMode wrap);

    static GPU_TEXTURE_FILTER_PARAM GetCitroFilterMode(love::Texture::FilterMode mode);

    static GPUFilter GetCitroFilterMode(const love::Texture::Filter& mode);

    static bool GetConstant(love::PixelFormat in, GPU_TEXCOLOR& out);

    static bool GetConstant(GPU_TEXCOLOR in, love::PixelFormat& out);

  private:
    GPUFilter filter;

    std::vector<std::function<void()>> deferredFunctions;
    std::vector<C3D_RenderTarget*> targets;
    C3D_RenderTarget* current;

    struct
    {
        GPU_TEXTURE_WRAP_PARAM s;
        GPU_TEXTURE_WRAP_PARAM t;
    } wrap;

    bool inFrame = false;

    void EnsureInFrame();
};
