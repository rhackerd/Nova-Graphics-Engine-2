#pragma once

#include "buffer.h"
#include "descMan.h"
#include "system.h"
#include <Nova/Core/structs.hpp>
#include <cglm/vec3.h>
namespace Nova::GE {

    #if defined(__AVX__)
        #define NOVA_MAT4_ALIGN 32
    #else
        #define NOVA_MAT4_ALIGN 16
    #endif

    struct alignas(NOVA_MAT4_ALIGN) CameraData {
        Nova::Core::Mat4 view;
        Nova::Core::Mat4 proj;
    };

    class Camera {
        public:
            Camera() = default;
            ~Camera() = default;

        public:
            void init(VmaAllocator allocator, DescriptorMan& descMan);
            void shutdown();

            void setPerspective(float fovDeg, float aspect, float nearP, float farP);
            void setOrthographic(float left, float right, float bottom, float top, float nearP, float farP);
            void setAspect(float aspect);

            void setPosition(const Nova::Core::Vec3& pos);
            void setTarget(const Nova::Core::Vec3& target);
            void setUp(const Nova::Core::Vec3& up);
            void setRotation(const Nova::Core::Quat& rot);

            void move(const Nova::Core::Vec3& delta);
            void rotate(const Nova::Core::Quat& delta);

            void update();

            const CameraData&      getData()        const { return m_data; }
            vk::Buffer             getBuffer()      const { return m_buffer.getBuffer(); }
            size_t                 getDescOffset()  const { return m_offset; }
            bool                   isValid()        const { return m_buffer.isValid(); }

            Nova::Core::Vec3       getPosition()    const { return m_pos; }
            Nova::Core::Vec3       getForward()     const;
            Nova::Core::Vec3       getRight()       const;
            Nova::Core::Vec3       getUp()          const;
            SetHandle              getHandle()      const { return handle; }

            void initDescriptor(DescriptorMan& man, SetHandle& handle) {
                handle = man.allocateSet(handle.layout, handle.setIndex);
                man.writeUBO(handle, handle.setIndex, m_buffer.getBuffer(), sizeof(CameraData));
            }; 

        private:
            void rebuildView();

        private:
            Buffer              m_buffer;
            size_t              m_offset    = 0;
            CameraData          m_data;

            Nova::Core::Vec3    m_pos       = {0.0f, 0.0f, 0.0f};
            Nova::Core::Vec3    m_target    = {0.0f, 0.0f, 0.0f};
            Nova::Core::Vec3    m_up        = {0.0f, 1.0f, 0.0f};
            Nova::Core::Quat    m_rotation;

            enum class ProjType {Perspective, Orthographic} m_projType = ProjType::Perspective;
            float               m_fovDeg    = 60.0f;
            float               m_aspect    = 16.0f / 9.0f;
            float               m_nearP     = 0.1f;
            float               m_farP      = 1000.0f;
            float               m_left      = -1.0f;
            float               m_right     = 1.0f;
            float               m_top       = 1.0f;
            float               m_bottom    = -1.0f;

            SetHandle           handle;

            bool m_dirty = true;
    };
};