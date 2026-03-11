#pragma once

#include "system.h"
#include "types.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    namespace CreateInfo {
        struct UniformBuffer {
            u32 binding;
            u32 offset;
            vk::ShaderStageFlags stages;
            // removed size - sizeof(T) handles it
        };
    };

    using UBO = vk::Buffer;
    class UniformBuffer {
    public:
        UniformBuffer() {}
        ~UniformBuffer() { shutdown(); }

        UniformBuffer(const UniformBuffer&)            = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;

        UniformBuffer(UniformBuffer&& o) noexcept
            : m_device(o.m_device), m_allocator(o.m_allocator),
              m_buffer(o.m_buffer), m_allocation(o.m_allocation), m_info(o.m_info)
        {
            o.m_buffer     = nullptr;
            o.m_allocation = nullptr;
        }


        template<typename T>
        void write(const T& data) {
            memcpy(m_info.pMappedData, &data, sizeof(T));
        }
 
        template<typename T>
        vk::DescriptorBufferInfo descriptorInfo() const {
            return { m_buffer, 0, sizeof(T) };
        }

        vk::Buffer buffer() const { return m_buffer; }



    public:

        template<typename T>
        void init(vk::Device device, CreateInfo::UniformBuffer createInfo, VmaAllocator allocator) {
            m_allocator = allocator;
            m_device    = device;

            VkBufferCreateInfo bufferInfo{
                .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size                  = sizeof(T),
                .usage                 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices   = nullptr,
            };

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

            VkBuffer rawBuffer;
            vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &rawBuffer, &m_allocation, &m_info);
            m_buffer = rawBuffer;
        }

        void shutdown() {
            if (m_buffer && m_allocation)
                vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        }

        vk::DeviceAddress getAddress() const {
            vk::BufferDeviceAddressInfo info{m_buffer};
            return m_device.getBufferAddress(info);
        };



    private:
        
        vk::Device        m_device;
        VmaAllocator      m_allocator;
        vk::Buffer        m_buffer;
        VmaAllocation     m_allocation;
        VmaAllocationInfo m_info;
        size_t offset = 0;
    };
}