#pragma once


#include "system.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/base.h>
#include <Nova/Core/log.h>
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

#include <Nova/Core/structs.hpp>

#include <Nova/Core/macros.h>

namespace Nova::GE {


    namespace CreateInfo {
        struct Image {
            vk::ImageCreateInfo info;
            VmaAllocator * allocator;
            vk::Device device;

            class Builder;
        };

        enum class ImageType {
            e1D = static_cast<int>(vk::ImageType::e1D),
            e2D = static_cast<int>(vk::ImageType::e2D),
            e3D = static_cast<int>(vk::ImageType::e3D)
        };


        class Image::Builder : public Nova::Core::Base::Builder<Image> {
            public:
                Builder() {
                    get().info.usage = vk::ImageUsageFlagBits::eSampled;
                    get().allocator = nullptr;
                }

                Builder& setAllocator(VmaAllocator * allocator) {
                    get().allocator = allocator;
                    return *this;
                }

                Builder& setDevice(vk::Device device) {
                    get().device = device;
                    return *this;
                }

                Builder& setType(ImageType type) {
                    get().info.imageType = static_cast<vk::ImageType>(type);
                    return *this;
                }

                Builder& setFormat(vk::Format format) {
                    get().info.format = format;
                    return *this;
                }

                Builder& setInitialLayout(vk::ImageLayout layout) {
                    get().info.initialLayout = layout;
                    return *this;
                }

                Builder& setExtent(Nova::Core::Vec2 size, uint32_t depth = 1) {
                    get().info.extent = vk::Extent3D{
                        static_cast<uint32_t>(size.x()), 
                        static_cast<uint32_t>(size.y()), 
                        depth
                    };
                    return *this;
                }
        };
    };

    
    struct ImageViewHandle {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;

        explicit operator bool() const {
            return index != UINT32_MAX;
        }
    };

    struct StoredImageView  {
        vk::ImageView view = VK_NULL_HANDLE;
        uint32_t generation = 0;
        vk::ImageAspectFlags aspect{};
    };

    class Image;

    using ImagePtr = std::shared_ptr<Image>;

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
    NOVA_LOG_DEF("Image");
    class Image {
        public:
            Image(CreateInfo::Image& createInfo) {
                init(createInfo);
            }
            Image() {};
            ~Image() { shutdown(); };

        public:
            // Manual Control
            bool init(CreateInfo::Image& createInfo);
            void shutdown();

            vk::Image getImage() { return image; }
            vk::Format getFormat() { return format; }
            [[nodiscard]]
            ImageViewHandle createView(vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);
            [[nodiscard]]
            vk::ImageView resolve(ImageViewHandle handle);

        public:
            // Getters
            NINTERNAL Nova::Core::Vec2& getExtent() { return extent; }
            NINTERNAL Image *getThis() { return this; }
            NINTERNAL VmaAllocation getAlloc() { return alloc; }

            
        private:
            
        private:
            vk::Image image;
            vk::Format format;
            VmaAllocation alloc;
            vk::ImageCreateInfo info;
            
            Nova::Core::Vec2 extent;
            VmaAllocator allocator;
            vk::Device device;

            std::vector<StoredImageView> views;
            uint32_t currentGeneration = 1;
    };

    
};
