#pragma once
#include "pipeline.h"
#include "layout.h"
#include "vulkan/vulkan.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {
    vk::PipelineLayout makePipelineLayout(vk::Device device, 
                                        vk::detail::DispatchLoaderDynamic& dld, 
                                        const BakedLayout& layout) {
        vk::PipelineLayoutCreateInfo ci{};
        ci.setSetLayouts(layout.setLayouts)
        .setPushConstantRanges(layout.pushConstants);
        return device.createPipelineLayout(ci, nullptr, dld);
    }
    void Pipeline::init(vk::Device device, vk::detail::DispatchLoaderDynamic& dld, CreateInfo::Pipeline& ci) {
        // First make the layout
        this->layout = makePipelineLayout(device, dld, ci.layout);

        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStateCount(ci.dynamicStates.size())
        .setDynamicStates(ci.dynamicStates);


        ci.vertexLayout
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&ci.binding)
        .setVertexAttributeDescriptionCount(ci.vertexAttribs.size())
        .setPVertexAttributeDescriptions(ci.vertexAttribs.data());

        vk::PipelineRenderingCreateInfoKHR renderingCI{};
        renderingCI.setColorAttachmentCount(1);
        vk::Format format = ci.format;
        vk::Format dFormat = ci.depthFormat;
        renderingCI.setColorAttachmentFormats(format);
        renderingCI.setDepthAttachmentFormat(dFormat);

        vk::GraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.setPNext(&renderingCI)
        .setStages(ci.shaders)
        .setPVertexInputState(&ci.vertexLayout)
        .setPInputAssemblyState(&ci.inputAssembly)
        .setPViewportState(&ci.viewportState)
        .setPRasterizationState(&ci.rasterizer)
        .setPMultisampleState(&ci.multisampling)
        .setPDepthStencilState(&ci.depthStencil)
        .setPColorBlendState(&ci.colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(this->layout)
        .setRenderPass(VK_NULL_HANDLE)
        .setSubpass(0)
        .setFlags(vk::PipelineCreateFlagBits::eDescriptorBufferEXT);

        auto result = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCI, nullptr, dld);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        if (result.value == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        this->m_pipeline = result.value;
        this->device = device;
    }

    void Pipeline::shutdown() {
        if (this->m_pipeline != VK_NULL_HANDLE) {
            device.destroyPipelineLayout(layout);
            device.destroyDescriptorSetLayout(setLayout);
            device.destroyPipeline(this->m_pipeline);
            this->m_pipeline = VK_NULL_HANDLE;
            this->layout = VK_NULL_HANDLE;
        }
    };
};