#pragma once
#include "core.h"
#include "layout.h"
#include "shader.h"
#include "texture.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/base.h>
#include <array>
#include <concepts>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace Nova::GE {

    namespace CreateInfo {
        struct Pipeline {
            std::vector<vk::PipelineShaderStageCreateInfo> shaders;
            BakedLayout layout;
            vk::PipelineVertexInputStateCreateInfo vertexLayout;
            vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
            vk::PipelineViewportStateCreateInfo viewportState;
            vk::PipelineRasterizationStateCreateInfo rasterizer;
            vk::PipelineMultisampleStateCreateInfo multisampling;
            vk::PipelineColorBlendAttachmentState colorBlendAttachment;
            vk::PipelineColorBlendStateCreateInfo colorBlending;
            vk::PipelineDepthStencilStateCreateInfo depthStencil;
            std::vector<vk::DynamicState> dynamicStates;
            vk::VertexInputBindingDescription binding;
            std::vector<vk::VertexInputAttributeDescription> vertexAttribs;
            vk::Format format;
            vk::Format depthFormat;
            class Builder;
        };

        template<typename T>
        concept HasVertexLayout = requires {
            {T::binding()} -> std::convertible_to<vk::VertexInputBindingDescription>;
            {T::attributes()} -> std::convertible_to<std::vector<vk::VertexInputAttributeDescription>>;      
        };

        class Pipeline::Builder : public Nova::Core::Base::Builder<Pipeline> {
            public:
                Builder& addShader(weakRef<Nova::GE::Shader> shader) {
                    get().shaders.push_back(shader.lock()->getStageInfo());
                    return *this;
                }
                template<typename T> requires HasVertexLayout<T>
                [[deprecated("Not recommended, and not supported anymore. Will not affect anything.")]]
                Builder& setVertexLayoutCustom() {
                    auto binding = T::binding();
                    auto attrs = T::attributes();
                    get().vertexLayout.setVertexBindingDescriptionCount(1)
                    .setPVertexBindingDescriptions(&binding)
                    .setVertexAttributeDescriptionCount(attrs.size())
                    .setPVertexAttributeDescriptions(attrs.data());
                    return *this;
                }
                Builder& setLayout(BakedLayout layout) {
                    get().layout = layout;
                    return *this;
                }
                Builder& addDynamicState(vk::DynamicState state) {
                    get().dynamicStates.push_back(state);
                    return *this;
                };
                Builder& setViewportState(u32 viewportCount, u32 scissorCount) {
                    get().viewportState.viewportCount = viewportCount;
                    get().viewportState.scissorCount = scissorCount;
                    return *this;
                }
                Builder& prepareDefaultVertexLayout() {
                    get().vertexAttribs.clear();
                    get().binding = Vertex::binding();
                    
                    auto attrs = Vertex::attributes();
                    get().vertexAttribs.insert(
                        get().vertexAttribs.end(), 
                        attrs.begin(), 
                        attrs.end()
                    );
                    
                    get().vertexLayout
                        .setVertexBindingDescriptionCount(1)
                        .setPVertexBindingDescriptions(&get().binding)
                        .setVertexAttributeDescriptionCount(get().vertexAttribs.size())
                        .setPVertexAttributeDescriptions(get().vertexAttribs.data());
                    return *this;
                }
                Builder& prepareDefaultInputAssembly() {
                    get().inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
                    get().inputAssembly.primitiveRestartEnable = false;
                    return *this;
                };
                Builder& prepareDefaultDynamicState(u32 viewportCount = 1, u32 scissorCount = 1) {
                    get().dynamicStates.push_back(vk::DynamicState::eViewport);
                    get().dynamicStates.push_back(vk::DynamicState::eScissor);
                    this->setViewportState(viewportCount, scissorCount);
                    return *this;  
                };
                Builder& prepareDefaultRasterizer() {
                    get().rasterizer.depthClampEnable = false;
                    get().rasterizer.rasterizerDiscardEnable = false;
                    get().rasterizer.polygonMode = vk::PolygonMode::eFill;
                    get().rasterizer.lineWidth = 1.0f;
                    get().rasterizer.cullMode = vk::CullModeFlagBits::eBack;
                    get().rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
                    get().rasterizer.depthBiasEnable = false;
                    return *this;
                }
                Builder& prepareDefaultMultisampling() {
                    get().multisampling.sampleShadingEnable = false;
                    get().multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
                    get().multisampling.minSampleShading = 1.0f;
                    get().multisampling.pSampleMask = nullptr;
                    get().multisampling.alphaToCoverageEnable = false;
                    get().multisampling.alphaToOneEnable = false;
                    return *this;
                }
                Builder& prepareDefaultColorBlending() {
                    this->prepareDefaultBlendOp();
                    get().colorBlending.attachmentCount = 1;
                    get().colorBlending.pAttachments = &get().colorBlendAttachment;
                    get().colorBlending.blendConstants[0] = 0.0f;
                    get().colorBlending.blendConstants[1] = 0.0f;
                    get().colorBlending.blendConstants[2] = 0.0f;
                    get().colorBlending.blendConstants[3] = 0.0f;
                    return *this;
                }
                Builder& prepareDefaultDepthStencil() {
                    get().depthStencil.depthTestEnable = true;
                    get().depthStencil.depthWriteEnable = true;
                    get().depthStencil.depthCompareOp = vk::CompareOp::eLess;
                    get().depthStencil.depthBoundsTestEnable = false;
                    get().depthStencil.minDepthBounds = 0.0f;
                    get().depthStencil.maxDepthBounds = 1.0f;
                    get().depthStencil.stencilTestEnable = false;
                    return *this;
                }
                Builder& prepareDefaultBlendOp() {
                    get().colorBlendAttachment.blendEnable = false;
                    get().colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
                    get().colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
                    get().colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
                    get().colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
                    get().colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                    get().colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                    get().colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
                    return *this;
                }

                Builder& prepareDefault() {
                    this->prepareDefaultVertexLayout();
                    this->prepareDefaultDynamicState();
                    this->prepareDefaultRasterizer();
                    this->prepareDefaultInputAssembly();
                    this->prepareDefaultMultisampling();
                    this->prepareDefaultColorBlending();
                    this->prepareDefaultDepthStencil();
                    return *this;
                }
                Builder& setSwapchainFormat(vk::Format format) {
                    get().format = format;
                    return *this;
                }
                Builder& setDepthFormat(vk::Format format) {
                    get().depthFormat = format;
                    return *this;
                }
        };
    };

    class Pipeline {
        public:
            Pipeline(vk::Device device, vk::detail::DispatchLoaderDynamic& dld, CreateInfo::Pipeline& ci) {
                init(device, dld, ci);
            };
            ~Pipeline() {shutdown();};
        public:
            void init(vk::Device device, vk::detail::DispatchLoaderDynamic& dld, CreateInfo::Pipeline& ci);
            void shutdown();

        public:
            vk::Pipeline get() { return m_pipeline; }
            vk::PipelineLayout getLayout() { return layout; }
            vk::DescriptorSetLayout getSetLayout() { return setLayout; }
            
        private:
            vk::Pipeline m_pipeline = VK_NULL_HANDLE;
            vk::PipelineLayout layout = VK_NULL_HANDLE;
            vk::Device device = VK_NULL_HANDLE;
            vk::DescriptorSetLayout setLayout = VK_NULL_HANDLE;
    };
};