#include "high-level/graphics.h"
#include "device.h"
#include "system.h"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_core.h>

namespace Nova::Graphics {
    bool Graphics::init(Nova::Desktop::Man& man, Nova::Desktop::Window& window) {
        auto sCI = Nova::GE::CreateInfo::System::Builder()
            .setAppName("ExampleApp")
            .setAppVersion(VK_MAKE_VERSION(1, 0, 0))
            .enableValidation()
            .addRecommendedValidationFeatures()
            .addExtensions(man.getExtensions())
            .build();
        if (!m_system.init(sCI)) return false;

        
        window.createSurface(m_system.getInstance());

        auto dCI = Nova::GE::CreateInfo::Device::Builder()
            .setSystem(&m_system)
            .pickPhysicalDevice()
            .enableBindless()
            .setSurface(window.getSurface())
            .enableDefaultExt()
            .enableDescriptorBuffer()
            .build();
        if(!m_device.init(dCI)) return false;

        auto scCI = Nova::GE::CreateInfo::Swapchain::Builder()
            .assignWindow(window)
            .setDevice(m_device)
            .setExtent({800,600}) // TODO: In window add getSize and much more getters
            .setSampling(vk::SampleCountFlagBits::e1)
            .build();
        if(!m_swapchain.init(scCI)) return false;

        m_window = &window;

        return true;
    }

    void Graphics::shutdown() {
        m_swapchain.shutdown();
        m_window->destroy(m_system.getInstance());
        m_device.shutdown();
        m_system.shutdown();
    };
};