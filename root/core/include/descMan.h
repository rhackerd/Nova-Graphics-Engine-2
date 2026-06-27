#pragma once

#include "core.h"
#include "system.h"
#include "uniformBuffer.h"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {

    struct SetHandle {
        usize baseOffset = 0;
        vk::DescriptorSetLayout layout = VK_NULL_HANDLE;
        u32 setIndex = 0;
    };

    struct DescriptorSetLayout {
        vk::DescriptorSetLayout layout = VK_NULL_HANDLE;
        u32 setIndex = 0;
    };

    struct DescripotorSetAllocation {
        vk::DescriptorSetLayout layout;
        u32 setIndex;
        usize baseOffset;
    };

    inline usize align(usize offset, usize alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    class DescriptorMan {
    public:
        DescriptorMan()  = default;
        ~DescriptorMan() = default;

        void init(VmaAllocator allocator, DevicePackage device, vk::PhysicalDeviceDescriptorBufferPropertiesEXT decsProps, size_t size = 1024 * 1024);
        void shutdown(VmaAllocator allocator);

        // ── Queries ───────────────────────────────────────────────────
        usize getCurrentOffset() const { return m_offset; }
        vk::Buffer buffer()      const { return m_buffer; }

        usize getSetSize(vk::DescriptorSetLayout setLayout) {
            vk::DeviceSize size = 0;
            m_device.getDescriptorSetLayoutSizeEXT(setLayout, &size, m_dld);
            return size;
        }

        usize getBindingOffset(vk::DescriptorSetLayout setLayout, u32 binding) {
            vk::DeviceSize offset = 0;
            m_device.getDescriptorSetLayoutBindingOffsetEXT(setLayout, binding, &offset, m_dld);
            return offset;
        }

        // ── Allocation ────────────────────────────────────────────────
        // Reserve space for a full descriptor set, returns base offset
        SetHandle allocateSet(vk::DescriptorSetLayout setLayout, u32 binding) {
            usize size = getSetSize(setLayout);
            usize base = align(m_offset, m_descProps.descriptorBufferOffsetAlignment);
            m_offset = base + size;
            return {base, setLayout, binding};
        }

        void writeUBO(const SetHandle& set, u32 binding, vk::Buffer ubo, usize size) {
            usize bindingOff = getBindingOffset(set.layout, binding);
            _writeUBO(ubo, size, set.baseOffset + bindingOff);
        };

        void writeSampler(const SetHandle& set, u32 binding, vk::Sampler sampler) {
            usize bindingOff = getBindingOffset(set.layout, binding);
            _writeSampler(sampler, set.baseOffset + bindingOff);
        }

        void writeSampledImage(const SetHandle& set, u32 binding, vk::ImageView view, vk::ImageLayout layout) {
            usize bindingOff = getBindingOffset(set.layout, binding);
            _writeSampledImage(view, layout, set.baseOffset + bindingOff);
        }

    private:
        void _writeSampler(vk::Sampler sampler, usize atOffset);
        void _writeSampledImage(vk::ImageView view, vk::ImageLayout layout, usize atOffset);
        void _writeUBO(vk::Buffer ubo, usize size, usize atOffset);

    private:
        void* ptr(usize offset) { return static_cast<char*>(m_ptr) + offset; }

        vk::Buffer     m_buffer     = VK_NULL_HANDLE;
        VmaAllocation  m_allocation = {};
        VmaAllocationInfo m_info    = {};
        void*          m_ptr        = nullptr;
        usize          m_offset     = 0;
        vk::Device     m_device;
        vk::detail::DispatchLoaderDynamic m_dld;

        vk::PhysicalDeviceDescriptorBufferPropertiesEXT m_descProps{};
    };
}