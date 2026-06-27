#pragma once


#include "system.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Desktop/core.h>
#include <Nova/Desktop/window.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Nova/Core/core.h>
#include <vk_mem_alloc.h>

#include "device.h"
#include <Nova/Core/structs.hpp>

namespace Nova::GE {


    namespace CreateInfo {
        struct Swapchain {
            Nova::GE::Device* device;
            Nova::Core::Vec2 extent;
            Nova::Desktop::Window* window;
            
            class Builder {
                private: Swapchain* info;
                public: Builder() : info(new Swapchain()) {}
                        ~Builder() {if (info != nullptr) delete info;}

                    Builder& setDevice(Nova::GE::Device& device) {
                        info->device = &device;
                        return *this;
                    }

                    Builder& setExtent(Nova::Core::Vec2 extent) {
                        info->extent = extent;
                        return *this;
                    }

                    Builder& assignWindow(Nova::Desktop::Window& win) {
                        info->window = &win;
                        return *this;
                    }
                    
                    
                    Swapchain build() {
                        Swapchain result = *info;
                        delete info;
                        info = nullptr;
                        return result;
                    }
            };
        };
    };

    /**
     * @class GfxEngine
     * @brief Main class for NGE Graphics Engine
     *
     * 
     * This class manages memory allocators, and other main graphics resources. 
     *
     * 
     * Usage:
     *    - Instantiate one GfxEngine per application instance
     *    - Create createInfo either using internal builder, or manually
     *    - Shutdown is managed manually by the Engine
     *
     * Responsibilities:
     *    - Memory allocators
     *    - Device creation
     *    - Device manager
     *    - Debugging utils
     *
     */
    class Swapchain {
        public:
            Swapchain(CreateInfo::Swapchain& createInfo) {
                init(createInfo);
            }
            Swapchain() {};
            ~Swapchain() { shutdown(); };

        public:
            // Manual Control
            bool init(CreateInfo::Swapchain& createInfo);
            void shutdown();

            bool recreate(); // For example on win resize - will shutdown and recreate optimally

        public:
            bool advanceFrame();
            uint32_t getCurrentImageIndex() const { return imageIndex; }
            vk::Image getCurrentImage() const { return images[imageIndex]; }
            vk::ImageView getCurrentImageView() const { return imageViews[imageIndex]; }
            vk::Image getCurrentDepthImage() const { return m_depthImages[imageIndex]; }
            vk::ImageView getCurrentDepthImageView() const { return m_depthImagesViews[imageIndex]; }



        public:
            vk::SwapchainKHR getSwapchain() const { return swapchain; }
            vk::Extent2D getExtent() const { return {static_cast<uint32_t>(extent[0]), static_cast<uint32_t>(extent[1])}; }
            vk::Format getFormat() const { return m_format; }
            std::vector<vk::Image> getImages() const { return images; }
            std::vector<vk::ImageView> getImageViews() const { return imageViews; }
            std::vector<vk::Image> getDepthImages() const { return m_depthImages; }
            uint32_t getDepthImageCount() const { return static_cast<uint32_t>(m_depthImages.size()); }
            std::vector<vk::ImageView> getDepthImageViews() const { return m_depthImagesViews; }
            vk::Semaphore getImageAvailableSemaphore() const { return imageAvailable; }

            uint32_t getImageIndex() const { return imageIndex; }
            
            vk::Format getDepthFormat() const { return m_depthFormat; }


        private:
            vk::SurfaceFormat2KHR chooseFormat();
            uint32_t              chooseOptimalImgCount();
            void                  setupQueueSharing(vk::SwapchainCreateInfoKHR& createInfo);
            vk::PresentModeKHR    choosePresentMode();
            void                  updateExtent();
    

            void retrieveSwapchainImgs();

            void setupColorBuffer();
            void setupDepthBuffer();  

        public: void handleRecreation();
            
        private:
            vk::SwapchainKHR swapchain;

            // Color buffer
            std::vector<vk::Image> images;
            std::vector<vk::ImageView> imageViews;
            vk::Format m_format;

            // Depth buffer
            std::vector<vk::Image> m_depthImages;
            std::vector<ImagePtr> m_depthImagesPtrs;
            std::vector<vk::ImageView> m_depthImagesViews;
            std::vector<VmaAllocation> m_depthAllocations;
            vk::Format m_depthFormat;

            uint32_t imageIndex = 0;
            uint32_t imageCount;

            vk::Semaphore imageAvailable;

            CI::Swapchain m_createInfo;
            
            Nova::Core::Vec2 extent;
            Nova::GE::Device* device;
            Nova::Desktop::Window* window;

        private:
            NOVA_LOG_DEF("Swapchain");
    };  
};