#pragma once

#include "types.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/base.h>
#include <Nova/Core/macros.h>
#include <memory>

namespace Nova::GE {
    namespace CreateInfo {
        struct CommandBuffer {
            u32 CommandBufferCount = 8;
            bool secondary = false;
            bool manual = false;
            vk::Device device;


            class Builder;
        };
        class CommandBuffer::Builder : public Nova::Core::Base::Builder<CommandBuffer> {
        public:
            Builder() { get().CommandBufferCount = 1; }
            Builder& setCommandBufferCount(u32 count) { get().CommandBufferCount = count; return *this; }
            Builder& setSecondary(bool secondary) { get().secondary = secondary; return *this; }
            Builder& setPrimary() { get().secondary = false; return *this; }
            Builder& setManual(bool manual) { get().manual = manual; return *this; }
            CommandBuffer build() { return std::move(get()); }
        };
    }


    class CommandBuffer {
    public:
        CommandBuffer(CreateInfo::CommandBuffer& createInfo);
        CommandBuffer() {}
        ~CommandBuffer() { shutdown(); }
    public:
        // Manual Control
        bool init(CreateInfo::CommandBuffer& createInfo);
        void shutdown();
        vk::CommandBuffer getCommandBuffer() { return commandBuffer; }
        bool isSecondary() { return secondary; }
        NINTERNAL void set(vk::CommandBuffer commandBuffer) { this->commandBuffer = commandBuffer; }
    private:
        vk::CommandBuffer commandBuffer;
        vk::Device device;
        bool secondary;
    };

    using CommandBufferPtr = std::shared_ptr<CommandBuffer>;
};