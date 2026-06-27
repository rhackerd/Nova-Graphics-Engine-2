#pragma once

#include "device.h"
#include "descMan.h"
#include "swapchain.h"
#include "system.h"
#include <Nova/Desktop/window.hpp>

namespace Nova::Graphics {
    class Graphics {
    public:
        Graphics(Nova::Desktop::Man& man, Nova::Desktop::Window& window) {
            init(man, window);
        }
        Graphics() {};
        ~Graphics() { shutdown(); }
    
    public:
        bool init(Nova::Desktop::Man& man, Nova::Desktop::Window& window);
        void shutdown();

        Nova::GE::Device&      getDevice()      { return m_device; }
        Nova::GE::Swapchain&   getSwapchain()   { return m_swapchain; }
        Nova::GE::DescriptorMan& getDescMan()   { return m_device.getDescriptorManager(); }

    private:
        Nova::GE::System    m_system;
        Nova::GE::Device    m_device;
        Nova::GE::Swapchain m_swapchain;
        Nova::Desktop::Window* m_window;
    };
}