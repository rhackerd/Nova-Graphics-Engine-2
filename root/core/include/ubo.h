#pragma once

#include "buffer.h"
#include "types.h"
#include "vulkan/vulkan.hpp"

namespace Nova::GE {
    template<typename T>
    class UBOHandle {
        public:
            u32 binding = 0;
            vk::ShaderStageFlags stages = {};
            Buffer buffer;
            size_t descOffset = 0;
            bool initiated = false;

        void upload(const T& data) {
            buffer.upload(&data, sizeof(T));
        };
    };
};