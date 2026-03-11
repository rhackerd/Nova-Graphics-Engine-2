#include "buffer.h"

namespace Nova::GE {
    void Buffer::init(CreateInfo::Buffer& createInfo) {
        m_allocator         = createInfo.allocator;
        m_size              = createInfo.size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size     = createInfo.size;
        bufferInfo.usage    = static_cast<VkBufferUsageFlags>(createInfo.usage);
        
        VmaAllocationCreateInfo cAllocInfo{};
        cAllocInfo.usage    = createInfo.memUsage;
        cAllocInfo.flags    = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer buffer;
        vmaCreateBuffer(m_allocator, &bufferInfo, &cAllocInfo, &buffer, &m_alloc, &m_allocInfo);
        m_buffer = buffer;
    }
    void Buffer::shutdown() {
        if(m_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_buffer, m_alloc);
            m_buffer = VK_NULL_HANDLE;
            m_alloc = VK_NULL_HANDLE;
        }
    }
    void Buffer::upload(const void* data, vk::DeviceSize size, vk::DeviceSize offset) {
        assert(m_allocInfo.pMappedData && "Buffer is not CPU visible or not mapped");
        assert(offset + size <= m_size && "Upload exceeds buffer size");
        std::memcpy(static_cast<char*>(m_allocInfo.pMappedData) + offset, data, size);
    }
};