#pragma once
#include "vulkan/vulkan.hpp"
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Nova/Core/core.h>
#include <vk_mem_alloc.h>

namespace Nova::GE {
    namespace CreateInfo {
        struct System {
            std::string appName = "Nova";
            const std::string engineName = "Nova Graphics Engine (NGE)";

            const uint32_t apiVersion = VK_API_VERSION_1_4;
            uint32_t appVersion = VK_MAKE_VERSION(1, 0, 0);
            const uint32_t engineVersion = VK_MAKE_VERSION(0, 1, 0);

            std::vector<const char*> extensions = {};
            std::vector<vk::ValidationFeatureEnableEXT> ValidationFeaturesEXT;
            bool validation = true;

            class Builder {
                private:
                    System* info;

                public:
                    Builder() : info(new System()) {}

                    ~Builder() {
                        if (info != nullptr) delete info;
                    }

                    Builder& setAppName(const std::string& name) {
                        info->appName = name;
                        return *this;
                    }

                    Builder& setAppVersion(const uint32_t ver) {
                        info->appVersion = ver;
                        return *this;
                    }

                    Builder& addExtension(const std::string& extension) {
                        info->extensions.push_back(extension.c_str());
                        return *this;
                    }

                    Builder& addExtensions(const std::vector<const char*>& extensions) {
                        info->extensions.insert(info->extensions.end(), extensions.begin(), extensions.end());
                        return *this;
                    }

                    Builder& addValidationFeatures(const std::vector<vk::ValidationFeatureEnableEXT>& features) {
                        info->ValidationFeaturesEXT = features;
                        return *this;
                    }

                    Builder& addValidationFeatures(const vk::ValidationFeatureEnableEXT& feature) {
                        info->ValidationFeaturesEXT.push_back(feature);
                        return *this;
                    }

                    Builder& setValidationBestPractices() {
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eBestPractices);
                        return *this;
                    }

                    Builder& setValidationDebugPrintf() {
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eDebugPrintf);
                        return *this;
                    }

                    Builder& setValidationGpuAssisted() {
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
                        return *this;
                    }

                    Builder& setValidationGpuAssistedReserveBindingSlot() {
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot);
                        return *this;
                    }

                    Builder& enableValidation() {
                        info->validation = true;  // ← add
                        info->extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                        return *this;
                    }

                    Builder& addRecommendedValidationFeatures() {
                        this->enableValidation();  // ← call enableValidation instead of duplicating
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eBestPractices);
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eDebugPrintf);
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eGpuAssisted);
                        info->ValidationFeaturesEXT.push_back(vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot);
                        return *this;
                    }

                    System build() {
                        System result = *info;
                        delete info;
                        info = nullptr;
                        return result;
                    }
            };
        };
    };

    /**
     * @class GfxEngine
     * @brief Main class for NGE Graphics Engine
     *
     * 
     * This class manages memory allocators, and other main graphics resources. 
     *
     * 
     * Usage:
     *    - Instantiate one GfxEngine per application instance
     *    - Create createInfo either using internal builder, or manually
     *    - Shutdown is managed manually by the Engine
     *
     * Responsibilities:
     *    - Memory allocators
     *    - Device creation
     *    - Device manager
     *    - Debugging utils
     *
     */
    class System {
        public:
            System(CreateInfo::System& createInfo) {
                init(createInfo);
            }
            System() {};
            ~System() { shutdown(); };

        public:
            // Manual Control
            bool init(CreateInfo::System& createInfo);
            void shutdown();

        public: // Getters
            vk::Instance getInstance() { return instance; }
            VmaAllocator getAllocator() { return allocator; }
            vk::detail::DispatchLoaderDynamic& getDld() { return dld; }

        private:
            vk::Instance instance;
            VmaAllocator allocator;
            vk::detail::DispatchLoaderDynamic dld;
            NOVA_LOG_DEF("System");
    };

    
};