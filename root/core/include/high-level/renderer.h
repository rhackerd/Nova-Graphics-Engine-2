#pragma once

#include "ImageSampler.h"
#include "camera.h"
#include "core.h"
#include "cpuf.h"
#include "device.h"
#include "high-level/graphics.h"
#include "high-level/object.h"
#include "pipeline.h"
#include "swapchain.h"
#include "vulkan/vulkan.hpp"
#include <vector>
namespace Nova::Graphics {

    using Camera = GE::Camera;
    template<typename T>
    using weakRef = GE::weakRef<T>;
    template<typename T>
    using ref = GE::ref<T>;

    struct GBuffer {
        ref<GE::Texture> albedo;
        ref<GE::Texture> normal;
        ref<GE::Texture> depth;
    };

    struct Scene {
        weakRef<Camera> camera;
        std::vector<weakRef<Object>> objects;
    };

    class Renderer {
        public:
            Renderer(Graphics& graphics) : m_graphics(graphics) {
                init(graphics);
            }
            ~Renderer() {
                shutdown();
            };
        public:
        
            void init(Graphics& graphics);
            void shutdown();

            void step(Scene& scene);

        public:
            [[nodiscard]]
            weakRef<GE::Camera> createCamera() {
                auto camera = GE::makeRef<GE::Camera>();
                camera->init(m_device->getAllocator(), m_device->getDescriptorManager());
                camera->initDescriptor(m_device->getDescriptorManager(), m_cameraLayout);
                m_cameras.push_back(camera);
                return camera;
            };
            [[nodiscard]]
            weakRef<Object> createObject() {
                auto object = GE::makeRef<Object>();
                object->init(m_device, m_device->getAllocator(), &m_device->getDescriptorManager(), m_texLayout, &sampler);
                m_objects.push_back(object);
                return object;
            }

        private:
            Graphics& m_graphics;
            Nova::GE::Camera* m_CurrentCamera = nullptr;
            
            Nova::GE::Render::CPUF m_cpuf;
            Nova::GE::weakRef<Nova::GE::Pipeline> m_pipeline;
            vk::PipelineLayout m_pipelineLayout;

            GE::SetHandle m_cameraLayout;
            GE::SetHandle m_texLayout;

            vk::Fence m_fence;
            std::vector<vk::Semaphore> m_renderFinished;
            Nova::GE::Render::RenderTarget m_renderTarget;
            Nova::GE::Render::CBSlot* m_lastSlot = nullptr;

            Nova::GE::Device* m_device = nullptr;
            Nova::GE::Swapchain* m_swapchain;

            std::vector<GE::ref<GE::Camera>> m_cameras;
            std::vector<GE::ref<Object>> m_objects;
            Nova::GE::ImageSampler sampler;

    };
};