#include "modules/window/Window.hpp"

#include <coreinit/energysaver.h>

namespace love
{
    Window::Window() : WindowBase("love.window.gx2")
    {
        this->setDisplaySleepEnabled(false);
    }

    Window::~Window()
    {
        this->close(false);
        this->graphics.set(nullptr);
    }

    void Window::close()
    {
        this->close(true);
    }

    void Window::close(bool allowExceptions)
    {
        if (this->graphics.get())
        {
            if (allowExceptions && this->graphics->isRenderTargetActive())
                throw love::Exception(E_WINDOW_CLOSING_RENDERTARGET_ACTIVE);

            this->graphics->unsetMode();
        }

        this->open = false;
    }

    bool Window::setWindow(int width, int height, WindowSettings* settings)
    {
        if (!this->graphics.get())
            this->graphics.set(Module::getInstance<Graphics>(Module::M_GRAPHICS));

        this->close();

        if (!this->createWindowAndContext(0, 0, width, height, 0))
            return false;

        if (this->graphics.get())
            this->graphics->setMode(width, height, width, height, false, false, 0);

        return true;
    }

    void Window::updateSettingsImpl(const WindowSettings& settings, bool updateGraphicsViewport)
    {
        if (updateGraphicsViewport && this->graphics.get())
        {
            double scaledw, scaledh;
            this->fromPixels(this->pixelWidth, this->pixelHeight, scaledw, scaledh);

            this->graphics->setViewport(0, 0, scaledw, scaledh);
        }
    }

    bool Window::onSizeChanged(int width, int height)
    {
        return false;
    }

    void Window::setDisplaySleepEnabled(bool enable)
    {
        enable ? IMEnableDim() : IMDisableDim();
    }

    bool Window::isDisplaySleepEnabled() const
    {
        uint32_t enabled;
        IMIsDimEnabled(&enabled);

        return enabled;
    }
} // namespace love
