#pragma once

#include "Image.h"
#include "core.h"
#include "types.h"
#include "vulkan/vulkan.hpp"
namespace Nova::GE {
    enum class GBufferAttachmentFlags : u32 {
        None     = 0,
        Albedo   = 1 << 0,
        Normal   = 1 << 1,
        Material = 1 << 2,
        Depth    = 1 << 3,

        // presets
        Full    = Albedo | Normal | Material | Depth,
        Minimal = Albedo | Depth,
        Shadow  = Depth,
        SSAO    = Normal | Depth,
    };

    struct GBufferDesc {
        u32 width, height;
        GBufferAttachmentFlags flags = GBufferAttachmentFlags::Full;
    };

    struct GBufferAttachments {
        ref<Image> image;
        ImageViewHandle view;
        vk::Format format;
        bool active = false;
        
        vk::ImageView getView() { return image->resolve(view); }
    };

    class GBuffer {
        private:
            
    };
};