#pragma once
#include "device.h"
#include "swapchain.h"
#include <SDL3/SDL.h>
#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include <vulkan/vulkan.hpp>

namespace Nova::GE::Render {

    class ImGuiContext {
    public:
        // instance and graphicsQueueFamily must be passed since Device
        // marks them as internal — expose via system.getInstance() and
        // device.getQueueFamilyIndex() or pass from outside
        void init(Device& device, Swapchain& swapchain, SDL_Window* window,
                  vk::Instance instance, uint32_t graphicsQueueFamily);
        void shutdown(Device& device);

        void beginFrame();
        void endFrame(vk::CommandBuffer cmd);
        void processEvent(SDL_Event& e);
        void updateTarget(vk::ImageView view, u32 width, u32 height) {
            this->view = view;
            this->width = width;
            this->height = height;
        };

        ImGuiIO* getIO() { return io; }

    private:
        VkDescriptorPool m_pool   = VK_NULL_HANDLE;
        VkFormat         m_format = VK_FORMAT_UNDEFINED;  // stored for pipeline rendering info lifetime
        vk::ImageView view;
        u32 width, height = 0;
        ImGuiIO* io;
    };

} // namespace Nova::GE::Render