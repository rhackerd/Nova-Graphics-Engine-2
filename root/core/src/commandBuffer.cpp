#include "commandBuffer.h"

namespace Nova::GE {
    bool CommandBuffer::init(CreateInfo::CommandBuffer& createInfo) {
        device = createInfo.device;
        secondary = createInfo.secondary;
        return true;
        // The next part (allocation) is handled by the device or manually
    }

    void CommandBuffer::shutdown() {}
};