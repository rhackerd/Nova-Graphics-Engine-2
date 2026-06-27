#include "high-level/renderer.h"
#include "ImageSampler.h"
#include "cpuf.h"
#include "vulkan/vulkan.hpp"

namespace Nova::Graphics {
    void Renderer::init(Graphics& graphics) {
        // pipeline
        m_device = &graphics.getDevice();

        auto vertex = m_device->createShader("shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);
        auto frag   = m_device->createShader("shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);

        // First layout
        Nova::GE::ShaderSet cameraSet;
        cameraSet.addUniformBuffer(0, vk::ShaderStageFlagBits::eVertex);

        Nova::GE::ShaderSet texSet;
        texSet.addSampler(0, vk::ShaderStageFlagBits::eFragment)
            .addSampledImage(1, vk::ShaderStageFlagBits::eFragment);

        auto cameraSetLayout = cameraSet.bake(m_device->getDevice(), m_device->getDld(), 0);
        auto texSetLayout    = texSet.bake(m_device->getDevice(), m_device->getDld(), 1);

        Nova::GE::ShaderLayout shaderLayout;
        shaderLayout.addPushConstant(sizeof(Nova::Core::Mat4), vk::ShaderStageFlagBits::eVertex);
        auto bakedLayout = shaderLayout.bake({cameraSetLayout, texSetLayout});

        // Then pipeline
        Nova::GE::CreateInfo::Pipeline pipelineCI = Nova::GE::CreateInfo::Pipeline::Builder()
            .prepareDefault()
            .addShader(vertex)
            .addShader(frag)
            .setLayout(bakedLayout)
            .setSwapchainFormat(graphics.getSwapchain().getFormat())
            .setDepthFormat(graphics.getSwapchain().getDepthFormat())
            .build();
        m_pipeline = m_device->createPipeline(pipelineCI);
        m_pipelineLayout = m_pipeline.lock()->getLayout();
        
        // layouts
        m_cameraLayout = cameraSetLayout;
        m_texLayout    = texSetLayout;

        //
        sampler.Init(m_device->getDevice());

        // fence
        vk::FenceCreateInfo fenceCI{};
        fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
        m_fence = m_device->getDevice().createFence(fenceCI);

        u32 imgCount = graphics.getSwapchain().getImages().size();
        m_renderFinished.resize(imgCount);
        for (auto& sem : m_renderFinished)
            sem = m_device->getDevice().createSemaphore({});

        // CPUF
        Nova::GE::Render::CreateInfo::CPUF cpufCI{};
        cpufCI.device = m_device;
        m_cpuf.init(cpufCI);
        m_cpuf.setClearColor({0.1f, 0.1f, 0.1f});

        m_renderTarget = Nova::GE::Render::createRenderTarget(graphics.getSwapchain());
        m_lastSlot = nullptr;


        m_swapchain = &graphics.getSwapchain();
    }

    void Renderer::step(Scene& scene) {
        vk::Device device = m_device->getDevice();
        device.waitForFences(m_fence, true, UINT64_MAX);

        if (m_lastSlot) m_lastSlot->inUse = false;

        vk::Semaphore imageAvailable = m_swapchain->getImageAvailableSemaphore();
        
        if (!m_swapchain->advanceFrame()) {
            // Resize

            for (auto camera : m_cameras)
                camera->setAspect(m_swapchain->getExtent().width / (float)m_swapchain->getExtent().height);
            return;
        }

        for (auto camera : m_cameras)
            camera->update();

        device.resetFences(m_fence);
        Nova::GE::Render::updateRenderTarget(m_renderTarget);


        m_lastSlot = m_cpuf.batchSubmit(m_renderTarget,
        {
            [&](Nova::GE::Render::Cmd cmd) {
                cmd.setSwapchain(*m_swapchain);
                cmd.bindDescriptorBuffer();
                cmd.bindPipeline(m_pipeline.lock());
                // cmd.bindSet(0, camera.getHandle(), pipeline.lock()->getLayout());
                // cmd.bindSet(1, tex.lock()->getHandle(), pipeline.lock()->getLayout());
                // cmd.pushConstant(objectPos, pipeline.lock()->getLayout());
                // cmd.drawMesh(mesh);
                auto camera = scene.camera.lock();
                // cmd.bindSet(0, camera->getHandle(), m_pipelineLayout);
                cmd.bindSet(camera->getHandle());
                // cmd.bindSet(0, camera->getHandle(), m_pipelineLayout);
                for (auto& objRef : scene.objects) {
                    auto obj = objRef.lock();
                    
                    auto& texSet = obj->getTexSet();
                    cmd.bindSet(texSet);
                    cmd.pushConstant(obj->getTransform(), m_pipelineLayout);
                    cmd.drawMesh(obj->getMesh());
                }
            }
        },{}
        );

        vk::Semaphore renderFinished = m_renderFinished[m_swapchain->getImageIndex()];
        vk::CommandBuffer cb = m_lastSlot->cmd->getCommandBuffer();

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo{};
        submitInfo.setWaitSemaphores(imageAvailable)
                  .setWaitDstStageMask(waitStage)
                  .setCommandBuffers(cb)
                  .setSignalSemaphores(renderFinished);
        m_device->getGraphicsQueue().submit(submitInfo, m_fence);

        vk::SwapchainKHR sc  = m_swapchain->getSwapchain();
        uint32_t         idx = m_swapchain->getImageIndex();
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphores(renderFinished).setSwapchainCount(1);
        presentInfo.pSwapchains   = &sc;
        presentInfo.pImageIndices = &idx;
        m_device->getPresentQueue().presentKHR(presentInfo);

    };

    void Renderer::shutdown() {
        if (m_device == nullptr) return;

        sampler.Shutdown();

        m_device->getDevice().destroyDescriptorSetLayout(m_cameraLayout.layout);
        m_device->getDevice().destroyDescriptorSetLayout(m_texLayout.layout);

        m_device->getDevice().waitIdle();
        for (auto camera : m_cameras) {
            camera->shutdown();
        }
        for (auto object : m_objects) {
            object->shutdown();
        }

        for (auto s : m_renderFinished)
            m_device->getDevice().destroySemaphore(s);
        m_device->getDevice().destroyFence(m_fence);
        m_device = nullptr; 
    }
};