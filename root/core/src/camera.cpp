#include "camera.h"
#include <cglm/quat.h>
#include <numbers>

namespace Nova::GE {
    void Camera::init(VmaAllocator allocator, DescriptorMan& descMan) {
        auto ci = CreateInfo::Buffer::Builder()
            .setAllocator(allocator)
            .setSize(sizeof(CameraData))
            .asUniform()
            .build();
        m_buffer.init(ci);
        m_dirty = true;
    }

    void Camera::shutdown() {
        if (m_buffer.isValid())
            m_buffer.shutdown();
    };

    void Camera::setPerspective(float fovDeg, float aspect, float nearP, float farP) {
        m_projType = ProjType::Perspective;
        m_fovDeg = fovDeg;
        m_aspect = aspect;
        m_nearP = nearP;
        m_farP = farP;
        m_dirty = true;
    }

    void Camera::setOrthographic(float left, float right, float bottom, float top, float nearP, float farP) {
        m_projType = ProjType::Orthographic;
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_nearP = nearP;
        m_farP = farP;
        m_dirty = true;
    }

    Nova::Core::Vec3 Camera::getForward() const {
    return m_rotation * Nova::Core::Vec3(0.0f, 0.0f, -1.0f);
    }

    Nova::Core::Vec3 Camera::getRight() const {
        return m_rotation * Nova::Core::Vec3(1.0f, 0.0f, 0.0f);
    }

    Nova::Core::Vec3 Camera::getUp() const {
        return m_rotation * Nova::Core::Vec3(0.0f, 1.0f, 0.0f);
    }

    void Camera::setAspect(float aspect) {
        m_aspect = aspect;
        m_dirty = true;
    };

    void Camera::setTarget(const Nova::Core::Vec3& target) {
        m_target = target;
        m_dirty = true;
    };

    void Camera::setPosition(const Nova::Core::Vec3& pos) {
        m_pos = pos;
        m_dirty = true;
    };

    void Camera::setUp(const Nova::Core::Vec3& up) {
        m_up = up;
        m_dirty = true;
    };

    void Camera::setRotation(const Nova::Core::Quat& rot) {
        m_rotation = rot;
        m_dirty = true;
    };

    void Camera::move(const Nova::Core::Vec3& delta) {
        m_pos += delta;
        m_dirty = true;
    };

    void Camera::rotate(const Nova::Core::Quat& delta) {
        m_rotation = delta * m_rotation;
        m_dirty = true;
    };

    void Camera::update() {
        if (!m_dirty) return;

        rebuildView();

        // Projection
        if (m_projType == ProjType::Perspective) {
            float fovRad = m_fovDeg * (std::numbers::pi / 180.0f);
            m_data.proj = Nova::Core::Mat4::perspective(fovRad, m_aspect, m_nearP, m_farP);

            m_data.proj[1][1] *= -1.0f; // fli p y for Vulkan
        } else {
            mat4 ortho;
            glm_ortho(m_left, m_right, m_bottom, m_top, m_nearP, m_farP, ortho);
            m_data.proj = Nova::Core::Mat4(ortho);
            m_data.proj[1][1] *= -1.0f;
        }

        m_buffer.upload(&m_data, sizeof(CameraData));
        m_dirty = false;
    }

    void Camera::rebuildView() {
        Nova::Core::Quat identity;
        glm_quat_identity(identity.q);

        float dot;
        bool hasRotation = !(
            m_rotation[0] == 0.0f &&
            m_rotation[1] == 0.0f &&
            m_rotation[2] == 0.0f &&
            m_rotation[3] == 1.0f
        );

        if (hasRotation) {
            Nova::Core::Mat4 t = Nova::Core::Mat4::translation(
                Nova::Core::Vec3(-m_pos.x(), -m_pos.y(), -m_pos.z())
            );
            Nova::Core::Mat4 r = m_rotation.toMat4().transposed();
            m_data.view = r * t;
        } else {
            Nova::Core::Vec3 target = m_pos + Nova::Core::Vec3(0.0f, 0.0f, -1.0f);
            if (!(m_target.x() == 0.0f && m_target.y() == 0.0f && m_target.z() == 0.0f))
                target = m_target;
            m_data.view = Nova::Core::Mat4::lookAt(m_pos, target, m_up);
        }
    }
};