#pragma once

#include "modules/graphics/graphics.h"
#include "citro2d/citro.h"

#define RENDERER_NAME    "citro3d"
#define RENDERER_VERSION "1.3.1"
#define RENDERER_VENDOR  "devkitPro"
#define RENDERER_DEVICE  "DMP PICA200"

enum class love::Graphics::Screen: uint8_t
{
    SCREEN_LEFT,
    SCREEN_RIGHT,
    SCREEN_BOTTOM,
    SCREEN_TOP,
    SCREEN_MAX_ENUM
};

namespace love::citro2d
{
    class Graphics : public love::Graphics
    {
        void GetDimensions(Screen screen, int * width, int * height) override;

        Screen GetActiveScreen() const override;

        std::vector<std::string> GetScreens() const override;

        void SetActiveScreen(Screen screen) override;
    
        RendererInfo GetRendererInfo() const override;

        void Clear(std::optional<Colorf> color, std::optional<int> stencil, std::optional<double> depth) override;

        void Present() override;

        void SetColor(Colorf color) override;

        Font * NewDefaultFont(int size, const Texture::Filter & filter) override;

        Font * NewFont(const Rasterizer & rasterizer, const Texture::Filter & filter = Texture::defaultFilter) override;

        /* Primitives */

        void Polygon(DrawMode mode, const Vector2 * points, size_t count) override;

        void Polyfill(const Vector2 * points, size_t count, u32 color, float depth);

        void Polyline(const Vector2 * points, size_t count);

        void Rectangle(DrawMode mode, float x, float y, float width, float height) override;

        void Rectangle(DrawMode mode, float x, float y, float width, float height, float rx, float ry) override {};

        void Rectangle(DrawMode mode, float x, float y, float width, float height, float rx, float ry, int points) override {};

        void Ellipse(DrawMode mode, float x, float y, float a, float b) override;

        void Ellipse(DrawMode mode, float x, float y, float a, float b, int points) override {};

        void Circle(DrawMode mode, float x, float y, float radius) override;

        void Circle(DrawMode mode, float x, float y, float radius, int points) override {};

        void Arc(DrawMode drawmode, ArcMode arcmode, float x, float y, float radius, float angle1, float angle2);

        void Arc(DrawMode drawmode, ArcMode arcmode, float x, float y, float radius, float angle1, float angle2, int points) {};

        void Points(const Vector2 * points, size_t count, const Colorf * colors, size_t colorCount) override {};

        void SetPointSize(float size) override {};

        void Line(float startx, float starty, float endx, float endy) override;

        /* End Primitives */

        void SetLineWidth(float width) override;

        void SetDefaultFilter(const Texture::Filter & filter);

        void SetScissor(const Rect & scissor) override;

        void SetScissor() override;

        /* Useless */

        void SetBlendMode(BlendMode mode, BlendAlpha alpha) override {};

        void SetColorMask(ColorMask mask) override {};
    };
}