#include "core.h"
#include "device.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/core.h>
#include <set>
#include <vector>

namespace Nova::GE {
    void Device::setupQueues() {
        // TODO: Implement
        mGraphicsQueue = mLogicalDevice.getQueue(indices.graphicsFamily.value(), 0, dld);
        mPresentQueue = mLogicalDevice.getQueue(indices.presentFamily.value(), 0, dld);
        if (indices.transferFamily.has_value()) {
            mTransferQueue = mLogicalDevice.getQueue(indices.transferFamily.value(), 0, dld);
        }else {
            mTransferQueue = mGraphicsQueue;
            NOVA_INFO(*log, "No dedicated transfer queue, using graphics queue");
        }
        NOVA_INFO(*log, "Queues are ready for execution!");
    }

    void Device::FillFamilyIndices(std::optional<vk::SurfaceKHR> surface) {
        NOVA_INFO(*log, "Gathering Queue Family Properties");
        
        const auto queueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();
        
        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
            const auto& queueFamily = queueFamilyProperties[i];

            // Skip families with no queues
            if (queueFamily.queueCount == 0) {
                continue;
            }

            // Graphics Queue
            if (!indices.graphicsFamily.has_value() && 
                (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                indices.graphicsFamily = i;
                NOVA_INFO(*log, "Found Graphics Queue at family {}", i);
            }

            // Present Queue
            if (surface.has_value() && !indices.presentFamily.has_value()) {
                VkBool32 presentSupport = mPhysicalDevice.getSurfaceSupportKHR(i, surface.value(), system->getDld());
                if (presentSupport) {
                    indices.presentFamily = i;
                    NOVA_INFO(*log, "Found Present Queue at family {}", i);
                }
            }

            // Dedicated Transfer Queue (prefer one without graphics)
            if (!indices.transferFamily.has_value() &&
                (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) &&
                !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                indices.transferFamily = i;
                NOVA_INFO(*log, "Found dedicated Transfer Queue at family {}", i);
            }
        }

        // Fallback: use graphics queue for transfer if no dedicated queue
        if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value()) {
            indices.transferFamily = indices.graphicsFamily.value();
            NOVA_INFO(*log, "Using Graphics Queue for transfers (fallback)");
        }

        // Fallback: use graphics queue for present if no dedicated present queue
        if (surface.has_value() && !indices.presentFamily.has_value() && indices.graphicsFamily.has_value()) {
            indices.presentFamily = indices.graphicsFamily.value();
            NOVA_INFO(*log, "Using Graphics Queue for presentation (fallback)");
        }

        if (indices.isComplete()) {
            NOVA_INFO(*log, "Found all required queues");
        } else {
            NOVA_ERROR(*log, "Failed to find all required queue families!");
        }
    }

    std::vector<vk::DeviceQueueCreateInfo> Device::getQueueCreateInfos() {
        std::set<uint32_t> unique = {
            indices.graphicsFamily.value(),
            indices.transferFamily.value()
        };
        if (indices.presentFamily.has_value())
            unique.insert(indices.presentFamily.value());

        std::vector<vk::DeviceQueueCreateInfo> infos;
        for (uint32_t family : unique) {
            vk::DeviceQueueCreateInfo info{};
            info.setQueueFamilyIndex(family)
                .setQueueCount(1)
                .setPQueuePriorities(&m_queuePriority);
            infos.push_back(info);
        }
        return infos;
    }

    void Device::printGPUInfo(std::vector<const char*> ext) {

        Version version;

        vk::PhysicalDeviceProperties2 deviceProperties = mPhysicalDevice.getProperties2(system->getDld());
        NOVA_INFO(*log, " ├▶ GPU Name: {}", static_cast<std::string>(deviceProperties.properties.deviceName));
        NOVA_INFO(*log, " ├▶ GPU Vendor: {}", static_cast<int>(deviceProperties.properties.vendorID));
        std::string type;
        switch (deviceProperties.properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                type = "Discrete";
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                type = "Integrated";
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                type = "Virtual";
                break;
            case vk::PhysicalDeviceType::eCpu:
                type = "CPU";
                break;
            default:
                type = "Unknown";
                break;
        }
        NOVA_INFO(*log, " ├▶ GPU Type: {}", type);
        std::string apiVersion = fmt::format("{}.{}", VK_VERSION_MAJOR(deviceProperties.properties.apiVersion), VK_VERSION_MINOR(deviceProperties.properties.apiVersion));
        if (VK_VERSION_PATCH(deviceProperties.properties.apiVersion) > 0) {
            apiVersion += fmt::format(".{}", VK_VERSION_PATCH(deviceProperties.properties.apiVersion));
        }
        version = GetVersion(deviceProperties.properties.apiVersion);
        NOVA_INFO(*log, " ├▶ GPU API Version: {}.{}.{}", version.major, version.minor, version.patch);

        NOVA_INFO(*log, " └▶ Extensions: ");
        if (ext.size() == 0) NOVA_INFO(*log, "  └─➤ None");
        for (const char* extension : ext) {
            if (extension != ext.back()) NOVA_INFO(*log, "  ├─➤ {}", extension);
            NOVA_INFO(*log, "  └─➤ {}", extension);
        }
    };

    
}