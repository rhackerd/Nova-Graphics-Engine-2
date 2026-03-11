#pragma once
#include "vulkan/vulkan.hpp"
#include <Nova/Core/structs.hpp>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <fstream>
namespace Nova::GE {
    struct Version {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
    };


    inline Version GetVersion(uint32_t ver) {
        return {VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver), VK_VERSION_PATCH(ver)};
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            printf("Failed to open file: %s\n", filename.c_str());    
        return {};}
        
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        
        return buffer;
    }

    template<typename T> 
    using ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    ref<T> makeRef(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    };

    template<typename T>
    using weakRef = std::weak_ptr<T>;


    struct Vertex {
        Nova::Core::Vec3 pos;
        Nova::Core::Vec3 normal;
        Nova::Core::Vec2 uv;

        static vk::VertexInputBindingDescription binding() {
            return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
        }

        static std::vector<vk::VertexInputAttributeDescription> attributes() {
            return {
                {0, 0, vk::Format::eR32G32B32Sfloat,  offsetof(Vertex, pos)},
                {1, 0, vk::Format::eR32G32B32Sfloat,  offsetof(Vertex, normal)},
                {2, 0, vk::Format::eR32G32Sfloat,     offsetof(Vertex, uv)}
            };
        }
    };

    struct DevicePackage {
        vk::Device device;
        vk::detail::DispatchLoaderDynamic dld;
    };
};