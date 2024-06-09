#include "Scene/Components/Transform3DComponent.h"

#include "Renderer/RenderThread.h"
#include "Scene/Components/NodeComponent.h"

namespace brr
{
    ////////////////////////////
    /// Transform3DComponent ///
    ////////////////////////////

    //TODO: Update entity transforms on SceneRenderer

    Transform3DComponent::Transform3DComponent()
    {
    }

    void Transform3DComponent::RegisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'Transform3DComponent::RegisterGraphics' when SceneRenderer is NULL.");
        m_render_entity_id = scene_renderer_proxy->CreateRenderEntity(*this);
    }

    void Transform3DComponent::UnregisterGraphics()
    {
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        assert(scene_renderer_proxy && "Can't call 'Transform3DComponent::UnregisterGraphics' when SceneRenderer is NULL.");
        scene_renderer_proxy->DestroyRenderEntity(*this);
        m_render_entity_id = render::EntityID::NULL_ID;
    }

    void Transform3DComponent::SetTransformationMatrix(const glm::mat4& matrix)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, m_scale_, m_rotation_, m_position_, skew, perspective);

        PropagateTransformChange();
    }

    void Transform3DComponent::SetTransform(const glm::vec3& position,
                                            const glm::fquat& rotation,
                                            const glm::vec3& scale)
    {
        m_position_ = position;
        m_rotation_ = rotation;
        m_scale_    = scale;
        PropagateTransformChange();
    }

    void Transform3DComponent::SetPosition(const glm::vec3& position)
    {
        m_position_ = position;
        PropagateTransformChange();
    }

    void Transform3DComponent::SetRotation(const glm::fquat& rotation)
    {
        m_rotation_ = rotation;
        PropagateTransformChange();
    }

    void Transform3DComponent::Translate(const glm::vec3& translation)
    {
        m_position_ += translation;
        PropagateTransformChange();
    }

    void Transform3DComponent::Rotate(const glm::fquat& rotation)
    {
        m_rotation_ = rotation * m_rotation_;
        PropagateTransformChange();
    }

    void Transform3DComponent::Rotate(float angle,
                                      const glm::vec3& axis)
    {
        glm::fquat rotation(angle, axis);
        m_rotation_ = rotation * m_rotation_;
        PropagateTransformChange();
    }

    void Transform3DComponent::SetScale(glm::vec3 scale)
    {
        m_scale_ = scale;
        PropagateTransformChange();
    }

    glm::vec3 Transform3DComponent::GetGlobalPosition() const
    {
        const glm::mat4& global_transf = GetGlobalMatrix();
        glm::vec3 position, scale, skew;
        glm::fquat rotation;
        glm::vec4 perspective;
        glm::decompose(global_transf, scale, rotation, position, skew, perspective);

        return position;
    }

    glm::fquat Transform3DComponent::GetGlobalRotation() const
    {
        const glm::mat4& global_transf = GetGlobalMatrix();
        glm::vec3 position, scale, skew;
        glm::fquat rotation;
        glm::vec4 perspective;
        glm::decompose(global_transf, scale, rotation, position, skew, perspective);

        return rotation;
    }

    glm::vec3 Transform3DComponent::GetGlobalScale() const
    {
        const glm::mat4& global_transf = GetGlobalMatrix();
        glm::vec3 position, scale, skew;
        glm::fquat rotation;
        glm::vec4 perspective;
        glm::decompose(global_transf, scale, rotation, position, skew, perspective);

        return scale;
    }

    glm::vec3 Transform3DComponent::GetGlobalRotationEuler() const
    {
        const glm::mat4& global_transf = GetGlobalMatrix();
        glm::vec3 position, scale, skew;
        glm::fquat rotation;
        glm::vec4 perspective;
        glm::decompose(global_transf, scale, rotation, position, skew, perspective);

        return glm::eulerAngles(rotation);
    }

    glm::mat4 Transform3DComponent::GetMatrix() const
    {
        glm::mat4 local_transform = glm::mat4_cast(m_rotation_);
        local_transform = glm::scale(local_transform, m_scale_);
        local_transform[3] = glm::vec4(m_position_, 1.0);
        //local_transform = local_transform * glm::translate(m_position_);
        return local_transform;
    }

    const glm::mat4& Transform3DComponent::GetGlobalMatrix() const
    {
        if (m_dirty_ & GLOBAL_DIRTY)
        {
            m_global_transform_ = GetMatrix();
            if (NodeComponent* parent = GetNodeComponent()->GetParentNode())
            {
                Transform3DComponent& parent_transform = parent->GetEntity().GetComponent<Transform3DComponent>();
                m_global_transform_                    = parent_transform.GetGlobalMatrix() * m_global_transform_;
            }
            m_dirty_ &= ~GLOBAL_DIRTY;
        }
        return m_global_transform_;
    }

    void Transform3DComponent::SetParent(Transform3DComponent* parent)
    {
        GetNodeComponent()->SetParent(parent->GetNodeComponent());
        PropagateTransformChange();
    }

    void Transform3DComponent::RemoveChild(Transform3DComponent* child)
    {
        assert((child != nullptr) && "You can't remove null child.");
        GetNodeComponent()->RemoveChild(child->GetNodeComponent());
        child->PropagateTransformChange();
    }

    void Transform3DComponent::PropagateTransformChange()
    {
        m_dirty_ |= DirtyFlags::GLOBAL_DIRTY;
        vis::SceneRenderProxy* scene_renderer_proxy = GetScene()->GetSceneRendererProxy();
        if (scene_renderer_proxy)
        {
            scene_renderer_proxy->UpdateRenderEntityTransform(*this);
        }

        for (NodeComponent* child : GetNodeComponent()->mChildren_)
        {
            Transform3DComponent& child_transform = child->GetEntity().GetComponent<Transform3DComponent>();
            child_transform.PropagateTransformChange();
        }
    }
}
