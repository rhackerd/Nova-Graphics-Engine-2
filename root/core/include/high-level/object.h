#pragma once

#include "ImageSampler.h"
#include "core.h"
#include "descMan.h"
#include "device.h"
#include "mesh.h"
#include "sampledImage.h"
#include "texture.h"
#include "vulkan/vulkan.hpp"
#include <Nova/Core/structs.hpp>
namespace Nova::Graphics {
    class Object {
    public:
        Object() {};
        ~Object() {};
        
    public:
        void loadMesh(const std::string& path) {
            m_mesh.init(path, allocator);
        };
        void loadTexture(const std::string& path) {
            m_texture = GE::makeRef<GE::Texture>();
            m_texture->init(path, *device, sampler->getSampler());
            sampler->Bind(*dMan, handle, *m_texture);
        }
        NINTERNAL void init(GE::Device* device, VmaAllocator allocator, GE::DescriptorMan* dMan, GE::SetHandle& setHandle, GE::ImageSampler* sampler) {
            this->device = device;
            this->allocator = allocator;
            this->dMan = dMan;
            this->layout = setHandle.layout;
            this->handle = setHandle;
            this->sampler = sampler;


            // Handle slots
            handle = dMan->allocateSet(handle.layout, handle.setIndex);
            sampler->BindSampler(*dMan, handle, 0);
        }


        const Nova::GE::SetHandle getTexSet() const { return handle; }
        Nova::GE::Mesh& getMesh()  { return m_mesh; }
        const Nova::Core::Mat4&     getTransform()     const { return m_model; }
        Nova::Core::Mat4&                        setTransform() {return m_model;}
        GE::SetHandle& getHandle() { return handle; }

        void shutdown() {
            m_mesh.shutdown();
            m_texture = nullptr;
        };

    private:
        VmaAllocator allocator;
        GE::Device* device;
        GE::DescriptorMan* dMan;
        vk::DescriptorSetLayout layout;

        GE::SetHandle handle;

        Nova::GE::Mesh m_mesh;
        Nova::GE::ref<Nova::GE::Texture> m_texture;
        Nova::Core::Mat4 m_model;
        Nova::GE::ImageSampler* sampler;
    };
};