#pragma once


#include "system.h"
#include "device.h"
namespace Nova::Graphics {
    class Graphics {
        private:
            Nova::GE::System system;
            Nova::GE::Device device;
    }; 
};