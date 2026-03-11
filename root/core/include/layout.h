#pragma once

#include "types.h"
#include "ubo.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/base.h>
#include <vector>
namespace Nova::GE {
    struct UniformBufferEntry {
        u32 binding;
        vk::ShaderStageFlags stages;
    };
    struct PushConstantEntry {
        u32 offset;
        u32 size;
        vk::ShaderStageFlags stages;
    };

    struct SamplerEntry {
        u32 binding;
        vk::ShaderStageFlags stages;
    };

    struct SampledImageEntry {
        u32 binding;
        vk::ShaderStageFlags stages;
    };

    using UBOEntry = UniformBufferEntry;
    using PCEntry = PushConstantEntry;

    struct BakedLayout {
        std::vector<vk::DescriptorSetLayout> setLayouts;
        std::vector<vk::PushConstantRange>   pushConstants;
    };

    class ShaderSet {
    public:
        ShaderSet& addUniformBuffer(u32 binding, vk::ShaderStageFlags stages) {
            m_uboEntries.push_back({binding, stages});
            return *this;
        }
        ShaderSet& addSampler(u32 binding, vk::ShaderStageFlags stages) {
            m_samplerEntries.push_back({binding, stages});
            return *this;
        }
        ShaderSet& addSampledImage(u32 binding, vk::ShaderStageFlags stages) {
            m_sampledImageEntries.push_back({binding, stages});
            return *this;
        }

        vk::DescriptorSetLayout bake(vk::Device device, vk::detail::DispatchLoaderDynamic& dld) {
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
            for (auto& e : m_uboEntries)
                bindings.push_back({e.binding, vk::DescriptorType::eUniformBuffer, 1, e.stages});
            for (auto& e : m_samplerEntries)
                bindings.push_back({e.binding, vk::DescriptorType::eSampler, 1, e.stages});
            for (auto& e : m_sampledImageEntries)
                bindings.push_back({e.binding, vk::DescriptorType::eSampledImage, 1, e.stages});

            vk::DescriptorSetLayoutCreateInfo ci{};
            ci.setBindings(bindings)
            .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eDescriptorBufferEXT);
            return device.createDescriptorSetLayout(ci, nullptr, dld);
        }

    private:
        std::vector<UBOEntry>          m_uboEntries;
        std::vector<SamplerEntry>      m_samplerEntries;
        std::vector<SampledImageEntry> m_sampledImageEntries;
    };

    class ShaderLayout {
    public:
        ShaderLayout& addPushConstant(u32 size, vk::ShaderStageFlags stages) {
            m_pushConstants.push_back({m_pcOffset, size, stages});
            m_pcOffset += size;
            return *this;
        }

        BakedLayout bake(std::initializer_list<vk::DescriptorSetLayout> setLayouts) {
            BakedLayout result;
            result.setLayouts = setLayouts;
            for (auto& pc : m_pushConstants)
                result.pushConstants.push_back({pc.stages, pc.offset, pc.size});
            return result;
        }

    private:
        std::vector<PCEntry> m_pushConstants;
        u32 m_pcOffset = 0;
    };
};