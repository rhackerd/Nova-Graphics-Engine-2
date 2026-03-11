#include "system.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/core.h>

namespace Nova::GE {
    bool System::init(CreateInfo::System& createInfo) {
        NINFO("Initializing {} v{}.{}.{}", createInfo.appName, VK_VERSION_MAJOR(createInfo.appVersion), VK_VERSION_MINOR(createInfo.appVersion), VK_VERSION_PATCH(createInfo.appVersion));
        NINFO("Initiating System");
        NINFO("Initiating Dynamic DIspatcher");

        vk::detail::DynamicLoader dl;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

        dld.init(vkGetInstanceProcAddr);


        createInfo.extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        createInfo.extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

        // Make instance
        if (createInfo.validation) {
                createInfo.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                // validation is NOT an extension, it's a layer
            }

            const char* validationLayer = "VK_LAYER_KHRONOS_validation";

    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
        .setApiVersion(createInfo.apiVersion)
        .setApplicationVersion(createInfo.appVersion)
        .setEngineVersion(createInfo.engineVersion)
        .setPApplicationName(createInfo.appName.c_str());

        vk::InstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo
            .setPEnabledExtensionNames(createInfo.extensions)
            .setPApplicationInfo(
                &appInfo
            );

        instanceCreateInfo
            .setPEnabledExtensionNames(createInfo.extensions)
            .setPApplicationInfo(&appInfo);

        if (createInfo.validation) {
            instanceCreateInfo.setPEnabledLayerNames(validationLayer); // ← separate call
        }


        instance = vk::createInstance(instanceCreateInfo, nullptr, dld);

        if (instance == nullptr) {
            NERROR("Failed to create instance");
            return false;
        }

        NINFO("System has been initiated");
        NINFO(" ├▶ API Version: {}.{}.{}", VK_VERSION_MAJOR(createInfo.apiVersion), VK_VERSION_MINOR(createInfo.apiVersion), VK_VERSION_PATCH(createInfo.apiVersion));
        NINFO(" ├▶ App Version: {}.{}.{}", VK_VERSION_MAJOR(createInfo.appVersion), VK_VERSION_MINOR(createInfo.appVersion), VK_VERSION_PATCH(createInfo.appVersion));
        NINFO(" ├▶ Engine Version: {}.{}.{}", VK_VERSION_MAJOR(createInfo.engineVersion), VK_VERSION_MINOR(createInfo.engineVersion), VK_VERSION_PATCH(createInfo.engineVersion));
        NINFO(" ├▶ App Name: {}", createInfo.appName);
        NINFO(" └▶ Extensions: ");
        for (const auto& extension : createInfo.extensions) {
            // Check if last
            if (extension == createInfo.extensions.back()) {
                NINFO("  └─➤ {}", extension);
            }else {
                NINFO("  ├─➤ {}", extension);
            }
        }

        if (createInfo.validation) {
            NINFO(" - Validation enabled");
        }

        NINFO("Stepping up dispatcher for instance");
        dld.init(instance);
        NINFO("Stepped");

        return true;
    }

    void System::shutdown() {
        if (!instance) return;
        NINFO("Shutting down System");
        instance.destroy();
        instance = nullptr;
        NINFO("Shutting down");
    }
};