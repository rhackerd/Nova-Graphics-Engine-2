#pragma once

#include "Image.h"
#include "core.h"
#include "descMan.h"
#include "system.h"
#include <Nova/Core/log.h>
#include <Nova/Core/macros.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "stb_image.h"
#include "buffer.h"
#include "types.h"
#include "vulkan/vulkan.hpp"
namespace Nova::GE {
    // Too high level for createInfo

    class Texture {
        public:
            Texture() {};
            ~Texture() {};

        public:
            bool init(const std::string& path, VmaAllocator allocator, vk::Device& device,  vk::CommandBuffer cBuffer, vk::Queue queue);
            void shutdown();

        public:
            weakRef<Image> getImage() { return m_image; }
            vk::ImageView getImageView() { return m_image->resolve(m_view); }
            vk::Sampler getSampler() { return m_sampler; }
            SetHandle getHandle() { return handle; }

            void initDescriptor(DescriptorMan& man, vk::DescriptorSetLayout layout) {
                handle = man.allocateSet(layout);
                man.writeSampler(handle, 0, m_sampler);
                man.writeSampledImage(handle, 1, getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);
            };            

        private:
            vk::Device m_device = VK_NULL_HANDLE;
            ref<Image> m_image = VK_NULL_HANDLE;
            ImageViewHandle m_view;
            vk::Sampler m_sampler = VK_NULL_HANDLE;

            SetHandle handle;
    };
};