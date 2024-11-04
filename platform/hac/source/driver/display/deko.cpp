#include "driver/display/deko.hpp"

namespace love
{
    deko3d::deko3d() :
        transform {},
        device(dk::DeviceMaker {}.setFlags(DkDeviceFlags_DepthMinusOneToOne).create()),
        mainQueue(dk::QueueMaker { this->device }.setFlags(DkQueueFlags_Graphics).create()),
        textureQueue(dk::QueueMaker { this->device }.setFlags(DkQueueFlags_Graphics).create()),
        commandBuffer(dk::CmdBufMaker { this->device }.create()),
        swapchain {},
        images(CMemPool(this->device, GPU_USE_FLAGS, GPU_POOL_SIZE)),
        data(CMemPool(this->device, CPU_USE_FLAGS, CPU_POOL_SIZE)),
        code(CMemPool(this->device, SHADER_USE_FLAGS, SHADER_POOL_SIZE)),
        framebufferSlot(-1)
    {
        std::memset(&this->context, 0, sizeof(this->context));
    }

    void deko3d::initialize()
    {
        if (this->initialized)
            return;

        this->uniformBuffer       = this->data.allocate(TRANSFORM_SIZE, DK_UNIFORM_BUF_ALIGNMENT);
        this->transform.modelView = glm::mat4(1.0f);

        this->commands.allocate(this->data, COMMAND_SIZE);
        this->createFramebuffers();

        this->initialized = true;
    }

    deko3d::~deko3d()
    {
        this->destroyFramebuffers();
        this->uniformBuffer.destroy();
    }

    void deko3d::createFramebuffers()
    {
        const auto& info = getScreenInfo()[0];

        this->depthbuffer.create(info, this->device, this->images, true);

        for (size_t index = 0; index < this->targets.size(); ++index)
        {
            this->framebuffers[index].create(info, this->device, this->images, false);
            this->targets[index] = &this->framebuffers[index].getImage();
        }

        this->swapchain = dk::SwapchainMaker { this->device, nwindowGetDefault(), this->targets }.create();

        this->context.viewport = Rect { 0, 0, info.width, info.height };
        this->context.scissor  = Rect { 0, 0, info.width, info.height };

        this->setViewport(this->context.viewport);
        this->setScissor(this->context.scissor);
    }

    void deko3d::destroyFramebuffers()
    {
        if (!this->swapchain)
            return;

        this->mainQueue.waitIdle();
        this->textureQueue.waitIdle();

        this->commandBuffer.clear();
        this->swapchain.destroy();

        for (auto& framebuffer : this->framebuffers)
            framebuffer.destroy();

        this->depthbuffer.destroy();
    }

    void deko3d::ensureInFrame()
    {
        if (!this->inFrame)
        {
            this->commands.begin(this->commandBuffer);
            this->inFrame = true;
        }
    }

    void deko3d::clear(const Color& color)
    {
        this->commandBuffer.clearColor(0, DkColorMask_RGBA, color.r, color.g, color.b, color.a);
    }

    void deko3d::clearDepthStencil(int depth, uint8_t mask, double stencil)
    {
        this->commandBuffer.clearDepthStencil(true, depth, mask, stencil);
    }

    dk::Image& deko3d::getInternalBackbuffer()
    {
        return this->framebuffers[this->framebufferSlot].getImage();
    }

    void deko3d::useProgram(const dk::Shader& vertex, const dk::Shader& fragment)
    {
        // clang-format off
        this->commandBuffer.bindShaders(DkStageFlag_GraphicsMask, { &vertex, &fragment });
        this->commandBuffer.bindUniformBuffer(DkStage_Vertex, 0, this->uniformBuffer.getGpuAddr(), this->uniformBuffer.getSize());
        // clang-format off
    }

    void deko3d::bindFramebuffer(dk::Image& framebuffer)
    {
        if (!this->swapchain)
            return;

        this->ensureInFrame();
        bool bindingModified = false;

        if (this->context.boundFramebuffer != &framebuffer)
        {
            bindingModified                = true;
            this->context.boundFramebuffer = &framebuffer;
        }

        if (bindingModified)
        {
            dk::ImageView depth { this->depthbuffer.getImage() };
            dk::ImageView target { framebuffer };

            this->commandBuffer.bindRenderTargets(&target, &depth);
        }
    }

    void deko3d::bindBuffer(BufferUsage usage, DkGpuAddr buffer, size_t size)
    {
        if (usage == BUFFERUSAGE_VERTEX)
        {
            this->commandBuffer.bindVtxBuffer(0, buffer, size);
            return;
        }
        else if (usage == BUFFERUSAGE_INDEX)
        {
            this->commandBuffer.bindIdxBuffer(DkIdxFormat_Uint16, buffer);
        }
    }

    void deko3d::prepareDraw(GraphicsBase* graphics)
    {}

    void deko3d::present()
    {
        if (!this->swapchain)
            return;

        if (this->inFrame)
        {
            this->mainQueue.submitCommands(this->commands.end(this->commandBuffer));
            this->mainQueue.presentImage(this->swapchain, this->framebufferSlot);

            this->inFrame = false;
        }

        this->framebufferSlot = this->mainQueue.acquireImage(this->swapchain);
    }

    void deko3d::setVertexWinding(Winding winding)
    {
        DkFrontFace face;
        if (!deko3d::getConstant(winding, face))
            return;

        this->context.rasterizer.setFrontFace(face);
    }

    void deko3d::setCullMode(CullMode mode)
    {
        DkFace cullMode;
        if (!deko3d::getConstant(mode, cullMode))
            return;

        this->context.rasterizer.setCullMode(cullMode);
    }

    void deko3d::setColorMask(ColorChannelMask mask)
    {
        const auto red   = (DkColorMask_R * mask.r);
        const auto green = (DkColorMask_G * mask.g);
        const auto blue  = (DkColorMask_B * mask.b);
        const auto alpha = (DkColorMask_A * mask.a);

        this->context.colorWrite.setMask(0, (red + green + blue + alpha));
    }

    void deko3d::setBlendState(const BlendState& state)
    {
        if (this->context.blendState == state)
            return;

        DkBlendOp operationRGB;
        if (!deko3d::getConstant(state.operationRGB, operationRGB))
            return;

        DkBlendOp operationA;
        if (!deko3d::getConstant(state.operationA, operationA))
            return;

        DkBlendFactor sourceColor;
        if (!deko3d::getConstant(state.srcFactorRGB, sourceColor))
            return;

        DkBlendFactor destColor;
        if (!deko3d::getConstant(state.dstFactorRGB, destColor))
            return;

        DkBlendFactor sourceAlpha;
        if (!deko3d::getConstant(state.srcFactorA, sourceAlpha))
            return;

        DkBlendFactor destAlpha;
        if (!deko3d::getConstant(state.dstFactorA, destAlpha))
            return;

        this->context.blend.setColorBlendOp(operationRGB);
        this->context.blend.setAlphaBlendOp(operationA);

        // Blend factors
        this->context.blend.setSrcColorBlendFactor(sourceColor);
        this->context.blend.setSrcAlphaBlendFactor(sourceAlpha);

        this->context.blend.setDstColorBlendFactor(destColor);
        this->context.blend.setDstAlphaBlendFactor(destAlpha);
    }

    static DkScissor dkScissorFromRect(const Rect& rect)
    {
        DkScissor scissor {};

        scissor.x      = (uint32_t)rect.x;
        scissor.y      = (uint32_t)rect.y;
        scissor.width  = (uint32_t)rect.w;
        scissor.height = (uint32_t)rect.h;

        return scissor;
    }

    void deko3d::setScissor(const Rect& scissor)
    {
        this->ensureInFrame();
        DkScissor _scissor {};

        if (scissor == Rect::EMPTY)
            _scissor = dkScissorFromRect(this->context.scissor);
        else
            _scissor = dkScissorFromRect(scissor);

        this->commandBuffer.setScissors(0, { _scissor });
    }

    static DkViewport dkViewportFromRect(const Rect& rect)
    {
        DkViewport viewport {};

        viewport.x      = (float)rect.x;
        viewport.y      = (float)rect.y;
        viewport.width  = (float)rect.w;
        viewport.height = (float)rect.h;
        viewport.near   = Framebuffer::Z_NEAR;
        viewport.far    = Framebuffer::Z_FAR;

        return viewport;
    }

    void deko3d::setViewport(const Rect& viewport)
    {
        this->ensureInFrame();
        DkViewport _viewport {};

        if (viewport == Rect::EMPTY)
            _viewport = dkViewportFromRect(this->context.viewport);
        else
            _viewport = dkViewportFromRect(viewport);

        this->commandBuffer.setViewports(0, { _viewport });
    }

    deko3d d3d;
} // namespace love
