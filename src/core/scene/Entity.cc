
#include "Entity.h"

#include "Scene.h"
namespace engine {
Entity::Entity(entt::entity handle, Scene *scene)
    : m_EntityHandle(handle), scene_(scene) {}
} // namespace engine