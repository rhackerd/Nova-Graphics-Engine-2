#include "sampledImage.h"
#include "Image.h"
#include "core.h"
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    bool SampledImage::Init(Device& device, u32 width, u32 height, vk::Format format, vk::ImageUsageFlags usage, vk::Sampler sampler) {
        m_sampler = sampler;
        m_owned = true;
        auto imageCI = CreateInfo::Image::Builder()
            .setAllocator(&device.getAllocator())
            .setDevice(device.getDevice())
            .setFormat(format)
            .setExtent({(float)width, (float) height})
            .build();
        imageCI.info.setUsage(usage);
        imageCI.info.setImageType(vk::ImageType::e2D);

        m_image = makeRef<Image>(imageCI);
        m_view = m_image->createView(vk::ImageAspectFlagBits::eColor);
        return true;
    };

    bool SampledImage::InitFromExisting(ref<Image> image, ImageViewHandle view, vk::Sampler sampler) {
        m_sampler = sampler;
        m_owned = false;
        m_image = image;
        m_view = view;
        return true;
    }

    void SampledImage::Shutdown() {
        if (m_owned && m_image) m_image->shutdown();
        m_image = nullptr;
        m_sampler = VK_NULL_HANDLE;
    }
};