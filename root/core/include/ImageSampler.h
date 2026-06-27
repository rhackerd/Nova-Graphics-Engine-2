#pragma once

#include "descMan.h"
#include "sampledImage.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>
namespace Nova::GE {

    enum class SamplerPreset {
        eLinearRepeat,
        eNearestRepeat,
        eLinearClamp,
        eNearestClamp,
        eShadow
    };

    class ImageSampler {

        public:
            ImageSampler() {};
            ~ImageSampler() {};

        public:
            bool Init(vk::Device device, SamplerPreset preset = SamplerPreset::eNearestRepeat);
            void Shutdown();

            void BindSampler(DescriptorMan& man, SetHandle handle, u32 binding);
            void Bind(DescriptorMan& man, SetHandle handle, SampledImage& image, u32 binding = 0);

        public: 
            // Getters
            vk::Sampler getSampler() { return m_sampler; }

        private:
            vk::Sampler m_sampler = VK_NULL_HANDLE;
            vk::Device m_device = VK_NULL_HANDLE;
            u32 m_baseOffset = 0;
    };
};