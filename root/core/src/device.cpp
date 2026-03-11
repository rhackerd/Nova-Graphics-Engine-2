#include "device.h"
#include "Image.h"
#include "commandBuffer.h"
#include "core.h"
#include "descMan.h"
#include "shader.h"
#include "system.h"
#include "uniformBuffer.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/core.h>
#include <Nova/Core/macros.h>
#include <memory>
#include <Nova/Desktop/core.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_hpp_macros.hpp>
#include <Nova/Core/log.h>
#include <vulkan/vulkan_to_string.hpp>

namespace Nova::GE {

    uint16_t GPUIndex = 0;
    bool Device::init(CreateInfo::Device& createInfo) {
        if (!createInfo.mPhysicalDevice) {
            NINFO("Invalid physical device!");
            return false;
        }

        mPhysicalDevice = createInfo.mPhysicalDevice;
        system = createInfo.system;

        log = std::make_unique<Nova::Core::Logger>(fmt::format("GPU #{}", GPUIndex));
        printGPUInfo(createInfo.extensions);
        NOVA_INFO(*log, "Initiating Device");

        // ── Features chain ────────────────────────────────────────────
        vk::PhysicalDeviceDynamicRenderingFeatures dynamicRendering{};
        dynamicRendering.setDynamicRendering(true);

        vk::PhysicalDeviceSynchronization2Features sync2{};
        sync2.setSynchronization2(true);
        sync2.setPNext(&dynamicRendering);

        vk::PhysicalDeviceBufferDeviceAddressFeatures bda{};
        bda.setBufferDeviceAddress(true);
        bda.setPNext(&sync2);

        vk::PhysicalDeviceDescriptorBufferFeaturesEXT descBuffer{};
        descBuffer.setDescriptorBuffer(true);
        descBuffer.setPNext(&bda);

        vk::PhysicalDeviceFeatures2 deviceFeatures{};
        deviceFeatures.setPNext(&descBuffer); // head of chain

        // ── Queues ────────────────────────────────────────────────────
        NOVA_INFO(*log, "Preparing for device creation");
        FillFamilyIndices(createInfo.surface);
        NOVA_INFO(*log, "Filled Family Indices");

        // ── Device ───────────────────────────────────────────────────
        auto queueInfos = getQueueCreateInfos();
        vk::DeviceCreateInfo dcreateInfo{};
        dcreateInfo
            .setQueueCreateInfos(queueInfos)
            .setPEnabledFeatures(nullptr)  // must be null when using Features2
            .setPNext(&deviceFeatures)
            .setPEnabledExtensionNames(createInfo.extensions);

        NOVA_INFO(*log, "Making device create info");
        mLogicalDevice = mPhysicalDevice.createDevice(dcreateInfo, nullptr, system->getDld());
        if (!mLogicalDevice) {
            NOVA_INFO(*log, "Failed to create device");
            return false;
        }
        NOVA_INFO(*log, "Created device");

        dld.init(system->getInstance(), vkGetInstanceProcAddr);
        dld.init(mLogicalDevice);
        setupQueues();
        GPUIndex++;

        // ── Allocator ─────────────────────────────────────────────────
        VmaAllocatorCreateInfo allocCI{};
        allocCI.vulkanApiVersion = VK_API_VERSION_1_4;
        allocCI.physicalDevice   = mPhysicalDevice;
        allocCI.device           = mLogicalDevice;
        allocCI.instance         = system->getInstance();
        allocCI.flags           |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        if (vmaCreateAllocator(&allocCI, &mAllocator) != VK_SUCCESS) {
            NOVA_INFO(*log, "Failed to create allocator");
            return false;
        }

        // ── Command pool ──────────────────────────────────────────────
        NOVA_INFO(*log, "Making command pool.");
        vk::CommandPoolCreateInfo poolCI{};
        poolCI.setQueueFamilyIndex(indices.graphicsFamily.value())
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer); // ← fix
        commandPool = mLogicalDevice.createCommandPool(poolCI);
        NOVA_INFO(*log, "Created command pool.");

        // ── Descriptor sizing ─────────────────────────────────────────
        NOVA_INFO(*log, "Querying compatibility");
        vk::PhysicalDeviceDescriptorBufferPropertiesEXT descProps{};
        vk::PhysicalDeviceProperties2 props2{};
        props2.pNext = &descProps;
        mPhysicalDevice.getProperties2(&props2);

        uboDescsSize   = descProps.uniformBufferDescriptorSize;
        samplerDescSize = descProps.samplerDescriptorSize;
        ssboDescSize   = descProps.storageBufferDescriptorSize;
        imageDescSize = descProps.sampledImageDescriptorSize;
        NOVA_INFO(*log, "Done querying compatibility");

        // ── Descriptor manager ────────────────────────────────────────
        NOVA_INFO(*log, "Making descriptor manager");
        descriptorManager.init(mAllocator, {mLogicalDevice, getDld()}, descProps);
        NOVA_INFO(*log, "Created descriptor manager");

        return true;
    }

    void Device::shutdown() {
        if (!mLogicalDevice) return;
        NOVA_INFO(*log, "Shutting down Device");

        for (auto& image : images) {
            image->shutdown();
        }
        images.clear();

        for (auto& shader : shaders) {
            shader->shutdown();
        }
        shaders.clear();

        // Command Buffers
        for (auto& commandBuffer : commandBuffers) {
            commandBuffer->shutdown();
        }
        commandBuffers.clear();

        // Pipelines
        for (auto& pl : pipelines) {
            pl->shutdown();
        }
        pipelines.clear();

        // UBOs
        for (auto& ubo : ubos) {
            ubo->shutdown();
        }
        ubos.clear();

        // DescMan
        descriptorManager.shutdown(mAllocator);

        // Textures
        for (auto& tex : textures) {
            tex->shutdown();
        }
        textures.clear();

        // Command pool
        if (commandPool != VK_NULL_HANDLE) {
            mLogicalDevice.destroyCommandPool(commandPool);
            commandPool = VK_NULL_HANDLE;
        }

        // Destroy allocator
        vmaDestroyAllocator(mAllocator);
        mAllocator = nullptr;

        mLogicalDevice.destroy();
        mLogicalDevice = nullptr;
        NOVA_INFO(*log, "Shut down Device");
    }





    ImagePtr Device::createImage(CreateInfo::Image& createInfo) {
        createInfo.device = this->mLogicalDevice;
        createInfo.allocator = &this->mAllocator;

        auto image = std::make_shared<Image>();  // Create shared_ptr directly
        
        if (!image->init(createInfo)) {
            NOVA_INFO(*log, "Failed to create image");
            return nullptr;
        }
        
        NOVA_INFO(*log, "Created image");
        images.push_back(image);  // Store the shared_ptr
        return image;
    }

    void Device::removeImage(ImagePtr image) {
        NOVA_INFO(*log, "Removing image");
        image->shutdown();
        images.erase(std::remove(images.begin(), images.end(), image), images.end());
    }

    std::vector<ref<CommandBuffer>> Device::createCommandBuffers(CreateInfo::CommandBuffer& createInfo) {
        createInfo.device = mLogicalDevice;

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.setCommandBufferCount(static_cast<uint32_t>(createInfo.CommandBufferCount));
        allocInfo.setCommandPool(commandPool);
        allocInfo.setLevel(createInfo.secondary 
            ? vk::CommandBufferLevel::eSecondary 
            : vk::CommandBufferLevel::ePrimary);

        auto allocated = mLogicalDevice.allocateCommandBuffers(allocInfo);

        NOVA_INFO(*log, "Allocating {} {} command buffers", 
            createInfo.CommandBufferCount, 
            createInfo.secondary ? "secondary" : "primary");

        std::vector<ref<CommandBuffer>> result;
        result.reserve(createInfo.CommandBufferCount);

        for (auto& cb : allocated) {
            auto commandBuffer = std::make_shared<CommandBuffer>();
            if (!commandBuffer->init(createInfo)) {
                NOVA_INFO(*log, "Failed to init command buffer");
                return {};  // empty vector on failure
            }
            commandBuffer->set(cb);
            commandBuffers.push_back(commandBuffer);
            result.push_back(commandBuffer);
        }

        NOVA_INFO(*log, "Done allocating command buffers");
        return result;
    }

    weakRef<Shader> Device::createShader(const std::string& path, vk::ShaderStageFlagBits stage) {
        NOVA_INFO(*log, "Making {} shader from {}", vk::to_string(stage), path);
        auto shader = makeRef<Shader>(path, stage, &mLogicalDevice);

        NOVA_INFO(*log, "Created {} shader from {}", vk::to_string(stage), path);

        shaders.push_back(shader);
        return shader;
    }

    weakRef<Pipeline> Device::createPipeline(CreateInfo::Pipeline& createInfo) {
        NOVA_INFO(*log, "Making pipeline");
        auto pipeline = makeRef<Pipeline>(mLogicalDevice, getDld(), createInfo);
        NOVA_INFO(*log, "Made a pipeline");
        pipelines.push_back(pipeline);
        return pipeline;
    }

    weakRef<Texture> Device::createTexture(const std::string& path) {
        NOVA_INFO(*log, "Making texture from {}", path);
        auto texture = makeRef<Texture>();

        auto cbci = CreateInfo::CommandBuffer{};
        cbci.CommandBufferCount = 1;
        cbci.secondary = false;

        auto cb = this->createCommandBuffers(cbci).front();

        if (!texture->init(path, mAllocator, mLogicalDevice, cb->getCommandBuffer(), getGraphicsQueue())) {
            cb->shutdown();
            NOVA_INFO(*log, "Failed to make texture from {}", path);
            return {};
        }
        mLogicalDevice.waitIdle();
        cb->shutdown();
        
        textures.push_back(texture);
        NOVA_INFO(*log, "Made a texture from {}", path);
        return texture;
    }

};
