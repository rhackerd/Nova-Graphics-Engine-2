#pragma once
#include "device.h"
#include "swapchain.h"
#include "types.h"
#include "vulkan/vulkan.hpp"
#include <vector>

namespace Nova::GE::Render {

    class FrameSync {
    public:
        void init(Device& device, Swapchain& swapchain);
        void shutdown(Device& device);

        void beginFrame(Device& device);

        void refreshImageAvailable(Swapchain& swapchain);

        vk::Semaphore getImageAvailable()            const;
        vk::Semaphore getRenderFinished(u32 imageIndex) const;
        vk::Fence     getFence()                     const;

    private:
        vk::Fence                  m_fence;
        vk::Semaphore              m_imageAvailable;
        std::vector<vk::Semaphore> m_renderFinished;
    };

} // namespace Nova::GE::Render