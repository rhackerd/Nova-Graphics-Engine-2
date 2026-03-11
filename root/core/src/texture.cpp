#include "texture.h"
#include "Image.h"
#include "commandBuffer.h"
#include "vulkan/vulkan.hpp"
#include <filesystem>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    static Core::Logger logger("Texture");
    bool Texture::init(const std::string& path, VmaAllocator allocator, vk::Device& device, vk::CommandBuffer cBuffer, vk::Queue gQueue) {
        m_device = device;
        NOVA_INFO(logger, "CWD: {}", std::filesystem::current_path().string());
        NOVA_INFO(logger, "Loading: {}", path);
        int w,h,ch;
        stbi_set_flip_vertically_on_load(false);
        u8* pixels = stbi_load(path.c_str(), &w, &h, &ch, STBI_rgb_alpha);
        if (!pixels) {
            NOVA_INFO(logger, "Failed to load texture {} : {}", path, stbi_failure_reason());
            return false;
        };

        vk::DeviceSize imgSize = w * h * 4;

        // Upload pixels

        auto stagingCI = CreateInfo::Buffer::Builder()
            .setAllocator(allocator)
            .setSize(imgSize)
            .asStaging()
            .build();

        Buffer staging;
        staging.init(stagingCI);
        staging.upload(pixels, imgSize);
        stbi_image_free(pixels);

        // Create Image on GPU
        auto ImageCI = CreateInfo::Image::Builder()
            .setAllocator(&allocator)
            .setDevice(device)
            .setType(CreateInfo::ImageType::e2D)
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setExtent({(float)w, (float)h})
            .build();

        ImageCI.info.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
        ImageCI.info.initialLayout = vk::ImageLayout::eUndefined;
        ImageCI.info.mipLevels = 1;
        ImageCI.info.arrayLayers = 1;
        ImageCI.info.sharingMode = vk::SharingMode::eExclusive;

        m_image = std::make_shared<Image>(ImageCI);
        m_view = m_image->createView(vk::ImageAspectFlagBits::eColor);


        auto cb = cBuffer;

        cb.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        vk::ImageMemoryBarrier2 toTransfer{};
        toTransfer
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(m_image->getImage())
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits2::eNone)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .subresourceRange
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setLevelCount(1).setLayerCount(1);
        cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(toTransfer));

        vk::BufferImageCopy region{};
        region
            .setBufferOffset(0)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .imageSubresource
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setMipLevel(0).setBaseArrayLayer(0).setLayerCount(1);
        region.setImageOffset({0,0,0})
            .setImageExtent({(u32)w, (u32)h, 1});
        cb.copyBufferToImage(staging.getBuffer(), m_image->getImage(),
            vk::ImageLayout::eTransferDstOptimal, region);
        
        vk::ImageMemoryBarrier2 toShader{};
        toShader
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImage(m_image->getImage())
            .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits2::eShaderRead);
        toShader.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1).setLayerCount(1);
        cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(toShader));
        cb.end();

        vk::SubmitInfo sInfo{};
        sInfo.setCommandBuffers(cb);
        gQueue.submit(sInfo);
        device.waitIdle();
        staging.shutdown();
        
        // Sampler
        vk::SamplerCreateInfo samplerCI{};
        samplerCI
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(false)
            .setCompareEnable(false)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear);
        m_sampler = device.createSampler(samplerCI);

        return true;
    }

    void Texture::shutdown() {
        if (m_image)                      m_image->shutdown();
        if (m_sampler != VK_NULL_HANDLE)  m_device.destroySampler(m_sampler);
        m_image = nullptr;
        m_sampler = VK_NULL_HANDLE;
    }
};