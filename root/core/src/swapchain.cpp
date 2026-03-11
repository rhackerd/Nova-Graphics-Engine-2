#include "swapchain.h"
#include "Image.h"
#include "vulkan/vulkan.hpp"
#include "SDL3/SDL.h"
#include <Nova/Core/macros.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>

namespace Nova::GE {
    bool Swapchain::init(CreateInfo::Swapchain& createInfo) {
        device = createInfo.device;
        window = createInfo.window;
        m_createInfo = createInfo; // FIX: store for recreation

        NOVA_SUPPRESS_INTERNAL_BEGIN
        vk::SurfaceCapabilities2KHR caps = device->getPhysicalDevice()
            .getSurfaceCapabilities2KHR(window->getSurface(), device->getDld());
        auto& surfaceCaps = caps.surfaceCapabilities;
        if (surfaceCaps.currentExtent.width != UINT32_MAX) {
            extent = { (float)surfaceCaps.currentExtent.width, (float)surfaceCaps.currentExtent.height };
        }else {
            int width, height;
            SDL_GetWindowSize(&window->get(), &width, &height); // This should be later integrated to Nova Desktop
            extent = { (float)width, (float)height };
        }
        
        NOVA_SUPPRESS_INTERNAL_END

        vk::SurfaceFormat2KHR surfaceFormat = chooseFormat();
        m_format = surfaceFormat.surfaceFormat.format; // FIX: store as member vk::Format
        vk::PresentModeKHR pMode = choosePresentMode();
        extent = createInfo.extent;


        imageCount = chooseOptimalImgCount();
        vk::SwapchainCreateInfoKHR aci{};
        aci.setSurface(window->getSurface())
        .setMinImageCount(imageCount)
        .setImageFormat(m_format)            // FIX
        .setImageColorSpace(surfaceFormat.surfaceFormat.colorSpace)
        .setImageExtent({(uint32_t)extent[0], (uint32_t)extent[1]})
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        setupQueueSharing(aci);

        aci.setPreTransform(caps.surfaceCapabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(pMode)
        .setClipped(vk::True);

        swapchain = device->getDevice().createSwapchainKHR(aci, nullptr, device->getDld());

        retrieveSwapchainImgs();
        setupColorBuffer();
        setupDepthBuffer();

        vk::SemaphoreCreateInfo si{};
        imageAvailable = device->getDevice().createSemaphore(si, nullptr, device->getDld());

        return true;
    }

    void Swapchain::shutdown() {
        if (!swapchain) return;

        for (auto& img : m_depthImagesPtrs) {
            device->removeImage(img);
        }
        for (auto& view : imageViews) {
            device->getDevice().destroyImageView(view, nullptr, device->getDld());
        }

        // Destroy semaphore
        if (imageAvailable != VK_NULL_HANDLE) device->getDevice().destroySemaphore(imageAvailable, nullptr, device->getDld());

        device->getDevice().destroySwapchainKHR(swapchain, nullptr, device->getDld());
        swapchain = VK_NULL_HANDLE;
        m_depthImagesPtrs.clear(); // ADD THIS
        m_depthImages.clear();     // ADD THIS
        m_depthImagesViews.clear(); // ADD THIS
        m_depthAllocations.clear(); // ADD THIS
        images.clear();             // ADD THIS
    }

    void Swapchain::retrieveSwapchainImgs()
    {
        images = device->getDevice().getSwapchainImagesKHR(swapchain, device->getDld());
    }

    void Swapchain::setupColorBuffer() {
        imageViews.resize(images.size());

        for (size_t i = 0; i < images.size(); i++)
        {
            vk::ImageViewCreateInfo ci{};
            ci.setImage(images[i])
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(m_format)
            .setComponents(vk::ComponentSwizzle::eIdentity)
            .subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1);

            imageViews[i] = device->getDevice().createImageView(ci, nullptr, device->getDld());      
        }
        NINFO("Color buffer is ready.");
    };

    void Swapchain::setupDepthBuffer() {
        // First find depth format
        std::vector<vk::Format> candidates = {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint 
        };
        vk::Format format = vk::Format::eUndefined;
        for (vk::Format candidate : candidates) {
            vk::FormatProperties props = device->getPhysicalDevice()
                .getFormatProperties(candidate);
            if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                format = candidate;
                break;
            }
        }

        if (format == vk::Format::eUndefined)
            throw std::runtime_error("No depth format found!");
        
        NINFO("Depth format: {}", vk::to_string(format));
        m_depthFormat = format;

        // now get image count

        m_depthImages.resize(images.size());
        m_depthImagesViews.resize(images.size());
        m_depthAllocations.resize(images.size());

        NINFO("Making depth buffer");
        for (size_t i = 0; i < images.size(); i++) {
            Nova::GE::CreateInfo::Image imageCI = Nova::GE::CreateInfo::Image::Builder()
                .setExtent(extent)
                .setType(CreateInfo::ImageType::e2D)
                .setFormat(format)
                .build();
            imageCI.info
                .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                .setTiling(vk::ImageTiling::eOptimal)
                .setFormat(format);

            ImagePtr image = device->createImage(imageCI);
            m_depthImagesPtrs.push_back(image);
            ImageViewHandle view = image->createView(vk::ImageAspectFlagBits::eDepth);
            m_depthImages[i] = image->getImage();
            m_depthImagesViews[i] = image->resolve(view);
        }

        NINFO("Made {} depth images successfully", images.size());
    };

    bool Swapchain::advanceFrame() {
        // Wait for previous frame to finish (if using fences)
        // device->getDevice().waitForFences(inFlightFence, VK_TRUE, UINT64_MAX);
        // device->getDevice().resetFences(inFlightFence);
        
        vk::AcquireNextImageInfoKHR anii{};
        anii.setSwapchain(swapchain);
        anii.setSemaphore(imageAvailable);  // Must be valid!
        anii.setTimeout(UINT64_MAX);
        anii.setDeviceMask(1);
        
        vk::Result res = device->getDevice().acquireNextImage2KHR(&anii, &imageIndex, device->getDld());
        
        // Handle special cases
        if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) {
            // Swapchain needs recreation (window resized, etc.)
            NINFO("Swapchain out of date, needs recreation");
            handleRecreation();
            return false;
        }
        
        if (res != vk::Result::eSuccess) {
            NERROR("Failed to acquire swapchain image: {}", vk::to_string(res));
            return false;
        }
        // print res every 10 or 20 frames
        static int frameCount = 0;
        if (frameCount % 10 == 0 && res != vk::Result::eSuccess) {
            NINFO("Acquired swapchain image {}", vk::to_string(res));
        }
        frameCount++;
        
        return true;
    }

    void Swapchain::handleRecreation() {
        int w, h;
        SDL_GetWindowSizeInPixels(&window->get(), &w, &h);
        while (w == 0 || h == 0) { // handle minimization
            SDL_GetWindowSizeInPixels(&window->get(), &w, &h);
            SDL_WaitEvent(nullptr);
        }

        vkDeviceWaitIdle(device->getDevice());
        shutdown();

        m_createInfo.extent = {static_cast<float>(w), static_cast<float>(h)}; // update extent
        init(m_createInfo);           // FIX: use stored createInfo
        extent = {static_cast<float>(w), static_cast<float>(h)};
    }
};
