#include "cpuf.h"
#include "commandBuffer.h"
#include "core.h"
#include "device.h"
#include "swapchain.h"
#include "vulkan/vulkan.hpp"
#include <array>

namespace Nova::GE::Render {

    void CPUF::init(CreateInfo::CPUF& createInfo) {
        m_device = createInfo.device;
        this->m_workers = createInfo.numPools;

        auto family = m_device->getIndices().graphicsFamily.value();
        m_pools.resize(m_workers);
        for (auto& slot : m_pools) {
            vk::CommandPoolCreateInfo ci{};
            ci.setQueueFamilyIndex(family)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
            slot.pool = m_device->getDevice().createCommandPool(ci);

            constexpr u32 secondariesPerPool = 2;
            slot.inUse.resize(secondariesPerPool, false);

            vk::CommandBufferAllocateInfo cbci{};
            cbci.setCommandBufferCount(secondariesPerPool)
                .setCommandPool(slot.pool)
                .setLevel(vk::CommandBufferLevel::eSecondary);
            auto scbs = m_device->getDevice().allocateCommandBuffers(cbci);
        
            slot.secondaries.resize(secondariesPerPool);
            for (u32 i = 0; i < secondariesPerPool; ++i) {
                slot.secondaries[i]->set(scbs[i]);
            }    
        }

        // Preallocate primaries
        auto primaryInfo = Nova::GE::CreateInfo::CommandBuffer::Builder()
            .setCommandBufferCount(3)
            .setPrimary()
            .build();
        auto primaryCmds = m_device->createCommandBuffers(primaryInfo);
        for (auto& cmd : primaryCmds)
            m_cmds.push_back({ false, cmd });

        // Preallocate secondaries
        auto secondaryInfo = Nova::GE::CreateInfo::CommandBuffer::Builder()
            .setCommandBufferCount(8)
            .setSecondary(true)
            .build();
        m_sCmds = m_device->createCommandBuffers(secondaryInfo);
    }

    Cmd CPUF::acquireSecondary(u32 poolIndex, u32 slotIndex, const vk::CommandBufferInheritanceInfo& inheritance) {
        auto& slot = m_pools[poolIndex];
        auto cb = slot.secondaries[slotIndex]->getCommandBuffer();
        cb.reset();
        Cmd cmd;
        cmd.init(cb, true);
        cmd.bindDevice(m_device);
        cmd.begin(&inheritance);
        return cmd;
    }

    void CPUF::releaseSecondary(Cmd& cmd) {
        cmd.end();
    }

CBSlot* CPUF::batchSubmit(RenderTarget& target, std::vector<std::function<void(Cmd)>> fns, std::function<void(vk::CommandBuffer)> after) {
        assert(fns.size() <= m_sCmds.size() && "Too many secondaries requested");

        // ----------------------------------------------------------------
        // Build inheritance info for secondaries (dynamic rendering)
        // ----------------------------------------------------------------
        vk::CommandBufferInheritanceRenderingInfo inheritanceRenderingInfo{};
        inheritanceRenderingInfo
            .setColorAttachmentFormats(target.format)
            .setDepthAttachmentFormat(target.depthFormat)
            .setRasterizationSamples(vk::SampleCountFlagBits::e1);

        vk::CommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.setPNext(&inheritanceRenderingInfo);

        // ----------------------------------------------------------------
        // Record secondaries
        // ----------------------------------------------------------------
        int secondaryIndex = 0;
        for (auto& fn : fns) {
            m_sCmds[secondaryIndex]->getCommandBuffer().reset();

            Cmd cmd;
            cmd.init(m_sCmds[secondaryIndex]->getCommandBuffer(), true);
            cmd.bindDevice(m_device);
            cmd.begin(&inheritanceInfo);
            fn(cmd);
            cmd.end();

            secondaryIndex++;
        }

        // ----------------------------------------------------------------
        // Find available primary
        // ----------------------------------------------------------------
        CBSlot* available = nullptr;
        for (auto& slot : m_cmds) {
            if (!slot.inUse) {
                available = &slot;
                break;
            }
        }
        assert(available && "No available primary command buffer");
        available->inUse = true;

        // ----------------------------------------------------------------
        // Record primary
        // ----------------------------------------------------------------
        available->cmd->getCommandBuffer().reset();
        Cmd primary;
        primary.init(available->cmd->getCommandBuffer(), false);
        primary.begin();

        // barrier: undefined → color attachment
        vk::ImageMemoryBarrier2 barrierAttach{};
        barrierAttach
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setImage(target.image)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite);
        barrierAttach.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1)
            .setLayerCount(1);

        vk::ImageMemoryBarrier2 depthBarrier{};
        depthBarrier
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setImage(target.depthImage)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite);
        depthBarrier.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eDepth)
            .setLevelCount(1)
            .setLayerCount(1);

        std::array barriers = { barrierAttach, depthBarrier };
        primary.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barriers));

        // beginRendering
        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment
            .setImageView(target.view)
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue{std::array<float, 4>{
                m_clearColor.x(), m_clearColor.y(), m_clearColor.z(), 1.0f}});

        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment
            .setImageView(target.depthView)
            .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setClearValue(vk::ClearDepthStencilValue{1.0f, 0});



        vk::RenderingInfo rInfo{};
        rInfo
            .setFlags(vk::RenderingFlagBits::eContentsSecondaryCommandBuffers)
            .setRenderArea(vk::Rect2D{{0, 0}, {(u32)target.extent.x(), (u32)target.extent.y()}})
            .setLayerCount(1)
            .setColorAttachments(colorAttachment)
            .setPDepthAttachment(&depthAttachment);
        primary.beginRendering(rInfo);

        // inject secondaries
        std::vector<vk::CommandBuffer> secondaryCbs;
        for (int i = 0; i < secondaryIndex; i++)
            secondaryCbs.push_back(m_sCmds[i]->getCommandBuffer());
        primary.get().executeCommands(secondaryCbs);

        primary.endRendering();

        if (after) after(primary.get());

        // barrier: color attachment → present
        vk::ImageMemoryBarrier2 barrierPresent{};
        barrierPresent
            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setImage(target.image)
            .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
            .setDstAccessMask(vk::AccessFlagBits2::eNone);
        barrierPresent.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1)
            .setLayerCount(1);
        primary.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrierPresent));

        primary.end();

        return available;
    }

    void CPUF::shutdown() {
        m_cmds.clear();
        m_sCmds.clear();
    };

};