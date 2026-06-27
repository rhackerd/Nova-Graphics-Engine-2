#include "texture.h"
#include "Image.h"
#include "commandBuffer.h"
#include "sampledImage.h"
#include "vulkan/vulkan.hpp"
#include <filesystem>
#include <memory>
#include <nova/logger/logger.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    static Core::Logger logger("Texture");
    bool Texture::init(const std::string path, Device& device, vk::Sampler sampler) {
        int w,h,ch;
        NOVA_INFO(logger, "Loading {}", path);
        u8* pixels = stbi_load(path.c_str(), &w, &h, &ch, STBI_rgb_alpha);
        if (!pixels) {
            NOVA_INFO(logger, "Failed to load {}", stbi_failure_reason());
            return false;
        }

        SampledImage::Init(device, w, h, vk::Format::eR8G8B8A8Srgb,
                   vk::ImageUsageFlagBits::eTransferDst |
                   vk::ImageUsageFlagBits::eSampled,
                   sampler);

        device.uploadToImage(getImage().lock(), pixels);
        stbi_image_free(pixels);
        return true;
    }
};