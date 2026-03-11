#include "descMan.h"
#include "system.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    void DescriptorMan::init(VmaAllocator allocator, DevicePackage device, vk::PhysicalDeviceDescriptorBufferPropertiesEXT descProps, size_t size) {
        VkBufferCreateInfo bufCI{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = size,
            .usage = VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        };
        VmaAllocationCreateInfo allocCI{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VkBuffer buffer;
        vmaCreateBuffer(allocator, &bufCI, &allocCI, &buffer, &m_allocation, &m_info);
        m_buffer = buffer;
        m_ptr    = m_info.pMappedData;

        m_device         = device.device;
        m_dld            = device.dld;
        m_descProps      = descProps;
        m_offset         = 0;
    }

    void DescriptorMan::shutdown(VmaAllocator allocator) {
        if (m_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, m_buffer, m_allocation);
            m_buffer     = VK_NULL_HANDLE;
            m_allocation = VK_NULL_HANDLE;
            m_ptr        = nullptr;
            m_offset     = 0;
        }
    }
    
    void DescriptorMan::_writeSampledImage(vk::ImageView view, vk::ImageLayout layout, usize atOffset) {
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.setImageView(view).setImageLayout(layout);

        vk::DescriptorDataEXT data{};
        data.pSampledImage = &imageInfo;

        vk::DescriptorGetInfoEXT getInfo{};
        getInfo.setType(vk::DescriptorType::eSampledImage).setData(data);

        m_device.getDescriptorEXT(&getInfo, m_descProps.sampledImageDescriptorSize,
        ptr(atOffset), m_dld);
    }

    void DescriptorMan::_writeSampler(vk::Sampler sampler, usize atOffset) {
        vk::DescriptorDataEXT data{};
        data.pSampler = &sampler;

        vk::DescriptorGetInfoEXT getInfo{};
        getInfo.setType(vk::DescriptorType::eSampler).setData(data);

        m_device.getDescriptorEXT(&getInfo, m_descProps.samplerDescriptorSize,
            ptr(atOffset), m_dld);
    }

    void DescriptorMan::_writeUBO(vk::Buffer ubo, usize size, usize atOffset) {
        vk::BufferDeviceAddressInfo addrInfo{ubo};
        auto addr = m_device.getBufferAddress(addrInfo);

        vk::DescriptorAddressInfoEXT bufInfo{};
        bufInfo.setAddress(addr).setRange(size).setFormat(vk::Format::eUndefined);

        vk::DescriptorDataEXT data{};
        data.pUniformBuffer = &bufInfo;

        vk::DescriptorGetInfoEXT getInfo{};
        getInfo.setType(vk::DescriptorType::eUniformBuffer).setData(data);

        // m_descProps.uniformBufferDescriptorSize // UBO size

        m_device.getDescriptorEXT(&getInfo, m_descProps.uniformBufferDescriptorSize,
            ptr(atOffset), m_dld);
    }
}