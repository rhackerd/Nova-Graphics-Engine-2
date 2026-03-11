#pragma once

#include "system.h"
#include <Nova/Core/base.h>
#include <vulkan/vulkan_core.h>
#include <cassert>

namespace Nova::GE {
    namespace CreateInfo {
        struct Buffer {
            VmaAllocator allocator;
            vk::DeviceSize size = 0;
            vk::BufferUsageFlags usage = vk::BufferUsageFlagBits{};
            VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY;

            class Builder;
        };

        class Buffer::Builder : Nova::Core::Base::Builder<Buffer> {
            public:
                Builder& setAllocator(VmaAllocator allocator) { get().allocator = allocator; return *this; }
                Builder& setSize(vk::DeviceSize size) { get().size = size; return *this; }
                Builder& setUsage(vk::BufferUsageFlags usage) { get().usage = usage; return *this; }
                Builder& setMemUsage(VmaMemoryUsage memUsage) { get().memUsage = memUsage; return *this; }

                // presets
                Builder& asVertex() { return setUsage(vk::BufferUsageFlagBits::eVertexBuffer).setMemUsage(VMA_MEMORY_USAGE_CPU_TO_GPU); }
                Builder& asIndex()  { return setUsage(vk::BufferUsageFlagBits::eIndexBuffer).setMemUsage(VMA_MEMORY_USAGE_CPU_TO_GPU);  }
                Builder& asUniform(){ 
                    return 
                    setUsage(vk::BufferUsageFlagBits::eUniformBuffer
                    | vk::BufferUsageFlagBits::eShaderDeviceAddress)
                    .setMemUsage(VMA_MEMORY_USAGE_CPU_TO_GPU);
                }
                Builder& asStaging(){ return setUsage(vk::BufferUsageFlagBits::eTransferSrc).setMemUsage(VMA_MEMORY_USAGE_CPU_TO_GPU);  }

                Buffer build() { return get(); }
        };
    };

    // This is maybe the best code i have written, or atleast the most clean one.

    // This class doesn't have a device create function
    // Because it should be managed by the Engine or class that uses it
    // For high level usage use things like UBO, Sample2D, etc...
    class Buffer {
        public:
            Buffer() = default;
            Buffer(CreateInfo::Buffer& createInfo) { init(createInfo); }
            ~Buffer() = default;
        public:
            // by usual order
            void init(CreateInfo::Buffer& createInfo);
            void upload(const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0);
            void shutdown();
        public:
            vk::Buffer      getBuffer() const   { return m_buffer;  }
            vk::DeviceSize  getSize()   const   { return m_size;    }
            VmaAllocation   getAlloc()  const   { return m_alloc;   }
            bool            isValid()   const   { return m_buffer != VK_NULL_HANDLE;}
        private:
            vk::Buffer          m_buffer        = VK_NULL_HANDLE;
            VmaAllocation       m_alloc         = VK_NULL_HANDLE;
            VmaAllocationInfo   m_allocInfo     = {};
            vk::DeviceSize      m_size          = 0;
            VmaAllocator        m_allocator     = nullptr;
    };
};