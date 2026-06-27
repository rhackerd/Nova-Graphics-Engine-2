#include "ImageSampler.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    bool ImageSampler::Init(vk::Device device, SamplerPreset preset) {
        m_device = device;

        vk::SamplerCreateInfo ci{};
        switch (preset) {
            case SamplerPreset::eLinearRepeat:
                ci.setMagFilter(vk::Filter::eLinear)
                    .setMinFilter(vk::Filter::eLinear)
                    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear);
                break;

            case SamplerPreset::eNearestRepeat:
                ci.setMagFilter(vk::Filter::eNearest)
                    .setMinFilter(vk::Filter::eNearest)
                    .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                    .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                    .setMipmapMode(vk::SamplerMipmapMode::eNearest);
                break;

            case SamplerPreset::eLinearClamp:
                ci.setMagFilter(vk::Filter::eLinear)
                    .setMinFilter(vk::Filter::eLinear)
                    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                    .setMipmapMode(vk::SamplerMipmapMode::eLinear);
                break;

            case SamplerPreset::eNearestClamp:
                ci.setMagFilter(vk::Filter::eNearest)
                    .setMinFilter(vk::Filter::eNearest)
                    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                    .setMipmapMode(vk::SamplerMipmapMode::eNearest);
                break;

            case SamplerPreset::eShadow:
                ci.setMagFilter(vk::Filter::eLinear)
                    .setMinFilter(vk::Filter::eLinear)
                    .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
                    .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
                    .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
                    .setCompareEnable(true)
                    .setCompareOp(vk::CompareOp::eLessOrEqual)
                    .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                    .setMipmapMode(vk::SamplerMipmapMode::eNearest);
                break;
        }

        m_sampler = m_device.createSampler(ci);
        return m_sampler != VK_NULL_HANDLE;
    }

    void ImageSampler::Shutdown() {
        if (m_sampler != VK_NULL_HANDLE) {
            m_device.destroySampler(m_sampler);
        }
        m_sampler = VK_NULL_HANDLE;
    }

    void ImageSampler::BindSampler(DescriptorMan& man, SetHandle handle, u32 binding) {
        man.writeSampler(handle, binding, m_sampler);
        m_baseOffset = binding + 1;
    }

    void ImageSampler::Bind(DescriptorMan& man, SetHandle handle, SampledImage& image, u32 binding) {
        u32 bind = binding == 0 ? m_baseOffset : binding;
        man.writeSampledImage(handle, bind, image.getImageView(),
                            vk::ImageLayout::eShaderReadOnlyOptimal);
        if (binding == 0) m_baseOffset++;
    }

    

    
};