#pragma once
#include "Image.h"
#include "buffer.h"
#include "camera.h"
#include "commandBuffer.h"
#include "core.h"
#include "mesh.h"
#include "pipeline.h"
#include "swapchain.h"
#include "types.h"
#include "vulkan/vulkan.hpp"
#include "Nova/Core/core.h"
#include "Nova/Core/base.h"
#include <array>
#include <cglm/types.h>
#include <functional>
#include <pthread.h>
#include <vector>
#include "device.h"
#include "texture.h"

namespace Nova::GE::Render {

    // -------------------------------------------------------------------------
    // CreateInfo
    // -------------------------------------------------------------------------
    namespace CreateInfo {
        struct CPUF {
            u32      secondaryCount = 8;
            Device*  device         = nullptr;
            u16 numPools = pthread_getconcurrency();

            class Builder;
        };

        class CPUF::Builder : Nova::Core::Base::Builder<CPUF> {
            public:
                Builder() { get().secondaryCount = 8; }
                Builder& setSecondaryCount(u32 count)   { get().secondaryCount = count; return *this; }
                Builder& setDevice(Device& device)      { get().device = &device;       return *this; }
                Builder& setNumPools(u16 num)           { get().numPools = num;         return *this; }
                CPUF build() { return get(); }
            };
    }


    struct ThreadSlot {
        vk::CommandPool pool;
        std::vector<ref<CommandBuffer>> secondaries;
        std::vector<bool> inUse;
    };

    // -------------------------------------------------------------------------
    // Cmd — thin vk::CommandBuffer wrapper, primary or secondary
    // -------------------------------------------------------------------------
    class Cmd {
    public:
        void init(vk::CommandBuffer cb, bool isSecondary = false) {
            m_cb          = cb;
            m_isSecondary = isSecondary;
        }

        void begin(const vk::CommandBufferInheritanceInfo* inheritance = nullptr) {
            vk::CommandBufferUsageFlags flags =
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            if (m_isSecondary) {
                flags |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
                assert(inheritance && "Secondary cmd buffer requires inheritance info");
            }
            vk::CommandBufferBeginInfo beginInfo{};
            beginInfo.setFlags(flags)
                     .setPInheritanceInfo(inheritance);
            m_cb.begin(beginInfo);
        }

        void bindPipeline(const ref<Pipeline>& pipeline) {
            m_cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->get());
            this->layout = pipeline->getLayout();
        }

        void draw(uint32_t vertexCount) {
            m_cb.draw(vertexCount, 1, 0, 0);
        }

        void pipelineBarrier2(const vk::DependencyInfo& depInfo) {
            m_cb.pipelineBarrier2(depInfo);
        };

        void beginRendering(const vk::RenderingInfo& rInfo) {m_cb.beginRendering(rInfo);}
        void endRendering() {m_cb.endRendering();}
        void setVkViewport(const vk::Viewport& viewport) { m_cb.setViewport(0, viewport); }
        void setScissor(const vk::Rect2D& scissor) { m_cb.setScissor(0, scissor); }
        void setSwapchain(const Nova::GE::Swapchain& swapchain) {
            auto extent = swapchain.getExtent();
            m_cb.setViewport(0, vk::Viewport(0, 0, 
                static_cast<float>(extent.width), 
                static_cast<float>(extent.height), 
                0.0f, 1.0f));
            m_cb.setScissor(0, vk::Rect2D(
                vk::Offset2D(0, 0), 
                vk::Extent2D(extent.width, extent.height)));  // uint32_t, no cast
        }
        void dynamicRendering(Nova::Core::Vec3 color,vk::RenderingAttachmentInfo info = {}) {
            if (info.imageView) {
                
            }
            if (color) info.setClearValue(vk::ClearColorValue{std::array<float, 4>{color.x(), color.y(), color.z(), 1.0f}});
        };

        void drawMesh(Mesh& mesh) {
            vk::Buffer buf = mesh.getVertexBuffer();
            vk::DeviceSize offset = 0;
            m_cb.bindVertexBuffers(0,1,&buf, &offset);
            m_cb.bindIndexBuffer(mesh.getIndexBuffer(), 0, vk::IndexType::eUint32);
            m_cb.drawIndexed(mesh.getIndexCount(), 1, 0, 0, 0);
        };

        void bindDevice(Device* device) {
            this->device = device;
        }

        void bindDescriptorBuffer() {
            vk::DescriptorBufferBindingInfoEXT bindingInfo{};
            vk::BufferDeviceAddressInfoEXT addressInfo{};
            addressInfo.setBuffer(device->getDescriptorManager().buffer());
            vk::DeviceAddress addr = device->getDevice().getBufferAddress(addressInfo, device->getDld());
            bindingInfo.setAddress(addr)
                .setUsage(vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT);
            m_cb.bindDescriptorBuffersEXT(bindingInfo, device->getDld());
        }

        void _bindSet(u32 setIndex, const SetHandle& set, vk::PipelineLayout layout) {
            u32 bufIdx = 0;
            vk::DeviceSize offset = set.baseOffset;
            m_cb.setDescriptorBufferOffsetsEXT(
                vk::PipelineBindPoint::eGraphics, layout,
                setIndex, 1, &bufIdx, &offset, device->getDld()
            );
        }

        void bindSet(const SetHandle& set) {
            _bindSet(set.setIndex, set, this->layout);
        }

        void pushConstant(const Nova::Core::Mat4& mat, vk::PipelineLayout layout) {
            m_cb.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(mat4), &mat);
        };

        void end() { m_cb.end(); }

        vk::CommandBuffer get() const { return m_cb; }

    private:
        vk::CommandBuffer m_cb          = VK_NULL_HANDLE;
        Device* device = nullptr;
        bool              m_isSecondary = false;
        vk::PipelineLayout layout;
    };

    struct RenderTarget {
        vk::Image       image;
        vk::ImageView    view;
        vk::Format       format;   // needed for inheritance info
        Nova::Core::Vec2 extent;
        vk::Image msImage;

        vk::Image depthImage;
        vk::ImageView depthView;
        vk::Format depthFormat;

        vk::ImageView msView;
        vk::SampleCountFlagBits sampleCount;

        Nova::GE::Swapchain* swapchain; // Needed only for update render Target 
    };

    // -------------------------------------------------------------------------
    // CBSlot — pool slot for a primary command buffer
    // -------------------------------------------------------------------------
    struct CBSlot {
        bool               inUse = false;
        ref<CommandBuffer> cmd;
    };

    inline RenderTarget createRenderTarget(Nova::GE::Swapchain& swapchain) {
        return RenderTarget{
            .image     = swapchain.getCurrentImage(),
            .view      = swapchain.getCurrentImageView(),
            .format    = swapchain.getFormat(),
            .extent    = {(float)swapchain.getExtent().width, (float)swapchain.getExtent().height},
            .msImage = swapchain.getCurrentMSColorImage(),
            .depthImage = swapchain.getCurrentDepthImage(),
            .depthView = swapchain.getCurrentDepthImageView(),
            .depthFormat = swapchain.getDepthFormat(),
            .msView = swapchain.getCurrentMSColorImageView(),
            .sampleCount = swapchain.getSampleCount(),
            .swapchain = &swapchain
        };
    }

    inline void updateRenderTarget(RenderTarget& t) {
        t.image  = t.swapchain->getCurrentImage();
        t.view   = t.swapchain->getCurrentImageView();
        t.extent = {(float)t.swapchain->getExtent().width, (float)t.swapchain->getExtent().height};
        t.depthImage = t.swapchain->getCurrentDepthImage();
        t.depthView = t.swapchain->getCurrentDepthImageView();
        t.msView = t.swapchain->getCurrentMSColorImageView();
        t.msImage = t.swapchain->getCurrentMSColorImage();
        t.sampleCount = t.swapchain->getSampleCount();
    }

    // -------------------------------------------------------------------------
    // CPUF — CPU frame recorder
    // -------------------------------------------------------------------------
    class CPUF {
    public:
        CPUF() = default;
        CPUF(CreateInfo::CPUF& createInfo) { init(createInfo); }

    public:
        void     init(CreateInfo::CPUF& createInfo);
        void     shutdown();

        // Records fns into secondaries, executes into an available primary.
        // Returns pointer to the CBSlot — caller is responsible for marking
        // inUse = false once GPU is done (after fence signal).
        CBSlot*  batchSubmit(RenderTarget& target, std::vector<std::function<void(Cmd)>> fns, std::function<void(vk::CommandBuffer)> after);

        void     setClearColor(Nova::Core::Vec3 color) { m_clearColor = color; }

        Cmd acquireSecondary(u32 poolIndex, u32 slotIndex, const vk::CommandBufferInheritanceInfo& inheritance);
        void releaseSecondary(Cmd& cmd);
        

        CBSlot* beginPrimary(RenderTarget& target);
        void executeSecondaries(CBSlot& primary, const std::vector<vk::CommandBuffer>& ready);
        void endPrimary(CBSlot& primary, std::function<void(vk::CommandBuffer)> submit);

        u8 poolCount() const { return static_cast<u32>(m_pools.size()); }

    private:
        Nova::Core::Vec3                    m_clearColor = { 0.0f, 0.0f, 0.0f };
        Device*                             m_device = nullptr;
        std::vector<CBSlot>                 m_cmds;   // primaries
        std::vector<ref<CommandBuffer>>     m_sCmds;  // secondaries
        u8                                  m_workers;
        std::vector<ThreadSlot>             m_pools;
    };

} // namespace Nova::GE::Render
