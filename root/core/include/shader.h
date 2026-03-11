#pragma once

#include "vulkan/vulkan.hpp"
#include <Nova/Core/base.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "core.h"

namespace Nova::GE {
    namespace CreateInfo {

        [[deprecated("CreateInfo for shaderModules are deprecated, use the class init() instead.")]];
        struct Shader {
            vk::ShaderModuleCreateInfo moduleCI;

            class Builder;
        };

        class Shader::Builder : public Nova::Core::Base::Builder<Shader> {
            public:
                Builder& loadFromPath(const std::string& path) {
                    auto code = readFile(path);
                    get().moduleCI.codeSize = code.size();
                    get().moduleCI.pCode = reinterpret_cast<const uint32_t*>(code.data());
                    return *this;
                };
        };
    };

    class Shader {
        public:
            Shader(const std::string& path, vk::ShaderStageFlagBits stage,vk::Device* device) {init(path, stage, device);}
            ~Shader() { shutdown(); }
        public:
            void init(const std::string& path, vk::ShaderStageFlagBits stage ,vk::Device* device) {
                auto code = readFile(path);
                vk::ShaderModuleCreateInfo moduleCi;
                moduleCi.codeSize = code.size();
                moduleCi.pCode = reinterpret_cast<const uint32_t*>(code.data());
                module = device->createShaderModule(moduleCi);
                this->stage = stage;
                this->device = device;
            }

            void shutdown() {
                if (module == VK_NULL_HANDLE) return;
                device->destroyShaderModule(module);
                module = VK_NULL_HANDLE;
            };

            vk::PipelineShaderStageCreateInfo getStageInfo() const {
                return vk::PipelineShaderStageCreateInfo()
                    .setStage(stage)
                    .setModule(module)
                    .setPName("main");
            };

        private:
            vk::Device* device;
            vk::ShaderStageFlagBits stage;
            vk::ShaderModule module;
    };
};