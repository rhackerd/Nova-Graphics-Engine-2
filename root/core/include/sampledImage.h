#pragma once


#include "Image.h"
#include "core.h"
#include "descMan.h"
#include "device.h"
#include "vulkan/vulkan.hpp"
namespace Nova::GE {

    // void uploadImage(Image&, void* pixels, vk::DeviceSize size, vk::CommandBuffer cmd, vk::Queue queue, VmaAllocator allocator);

    class SampledImage {
        public:
            bool Init(Nova::GE::Device& device, u32 width, u32 height, vk::Format format, vk::ImageUsageFlags usage, vk::Sampler sampler);
            void Shutdown();

            bool InitFromExisting(ref<Image> image, ImageViewHandle view, vk::Sampler sampler);



        public:
            //Getters
            vk::ImageView getImageView() { return m_image->resolve(m_view); }
            vk::Sampler getSampler() { return m_sampler; }
            SetHandle getHandle() { return m_setHandle; }
            weakRef<Image> getImage() { return m_image; }

        private:
            ref<Image> m_image;
            ImageViewHandle m_view;
            vk::Sampler m_sampler;
            SetHandle m_setHandle;
            bool m_owned = true;
    };
};