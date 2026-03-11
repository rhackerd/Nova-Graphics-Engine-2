#include "framesync.h"

namespace Nova::GE::Render {

    void FrameSync::init(Device& device, Swapchain& swapchain) {
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        m_fence = device.getDevice().createFence(fenceInfo);

        m_imageAvailable = swapchain.getImageAvailableSemaphore();

        vk::SemaphoreCreateInfo semInfo{};
        m_renderFinished.resize(swapchain.getImages().size());
        for (auto& sem : m_renderFinished)
            sem = device.getDevice().createSemaphore(semInfo);
    }

    void FrameSync::shutdown(Device& device) {
        device.getDevice().destroyFence(m_fence);
        for (auto& sem : m_renderFinished)
            device.getDevice().destroySemaphore(sem);
        m_renderFinished.clear();
    }

    void FrameSync::beginFrame(Device& device) {
        device.getDevice().waitForFences(m_fence, true, UINT64_MAX);
        device.getDevice().resetFences(m_fence);
    }

    void FrameSync::refreshImageAvailable(Swapchain& swapchain) {
        m_imageAvailable = swapchain.getImageAvailableSemaphore();
    }

    vk::Semaphore FrameSync::getImageAvailable() const {
        return m_imageAvailable;
    }

    vk::Semaphore FrameSync::getRenderFinished(u32 imageIndex) const {
        assert(imageIndex < m_renderFinished.size() && "Image index out of range");
        return m_renderFinished[imageIndex];
    }

    vk::Fence FrameSync::getFence() const {
        return m_fence;
    }

} // namespace Nova::GE::Render