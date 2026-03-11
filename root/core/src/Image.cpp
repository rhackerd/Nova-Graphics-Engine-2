#include "Image.h"
#include "system.h"
#include "vulkan/vulkan.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    bool Image::init(CreateInfo::Image& createInfo) {
        device = createInfo.device;
        allocator = *createInfo.allocator;


        vk::ImageCreateInfo info = createInfo.info;

        if (info.imageType == vk::ImageType::e1D) NINFO("1D images are uncommon, are you sure you want to use them?");
        if (info.format == vk::Format::eUndefined) 
        {
            info.format = vk::Format::eR8G8B8A8Unorm;
            NINFO("Image format is undefined, using R8G8B8A8Unorm");
        }
        if (info.extent.width == 0 || info.extent.height == 0 || info.extent.depth == 0) 
        {
            info.extent = vk::Extent3D{100, 100, 1};
            NINFO("Image extent is undefined, using 100x100x1");
        }
        if (info.mipLevels == 0) info.mipLevels = 1;
        if (info.arrayLayers == 0) info.arrayLayers = 1;

        // if (info.initialLayout == vk::ImageLayout::eUndefined) info.initialLayout = vk::ImageLayout::eGeneral;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        
        VkImage _image;

        VkResult result = vmaCreateImage(allocator, &static_cast<const VkImageCreateInfo&>(info), &allocInfo, &_image, &alloc, nullptr);

        if (result != VK_SUCCESS) {
            NERROR("Failed to create image");
            return false;
        }

        format = info.format;
        extent = {static_cast<float>(info.extent.width), static_cast<float>(info.extent.height)};
        this->info = info;

        image = vk::Image(_image);

        return true;
    }

    void Image::shutdown() {
        for (auto& v : views) {
            if (v.view != VK_NULL_HANDLE) {
                device.destroyImageView(v.view);
                v.view = VK_NULL_HANDLE;
            }
        }

        views.clear();
        currentGeneration++;

        if (alloc != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, image, alloc);
            alloc = VK_NULL_HANDLE;
        }
    }



    ImageViewHandle Image::createView(vk::ImageAspectFlags aspect) {
        for (uint32_t i = 0; i < views.size(); ++i) {
            if (views[i].aspect == aspect &&
                views[i].generation == currentGeneration) {
                return { i, currentGeneration };
            }
        }

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = image;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = info.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = info.arrayLayers;


        if (info.imageType == vk::ImageType::e2D && info.arrayLayers > 1)
            viewInfo.viewType = vk::ImageViewType::e2DArray;
        else
            viewInfo.viewType = vk::ImageViewType::e2D;

        vk::ImageView view = device.createImageView(viewInfo);

        views.push_back({
            .view = view,
            .generation = currentGeneration,
            .aspect = aspect
        });

        return { uint32_t(views.size() - 1), currentGeneration };
    }

    vk::ImageView Image::resolve(ImageViewHandle handle)  {
        assert(handle.index < views.size());

        const auto& stored = views[handle.index];
        assert(stored.generation == handle.generation);
        assert(stored.view != VK_NULL_HANDLE);

        return stored.view;
    }


};