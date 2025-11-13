
#include "Entity.h"

#include "Scene.h"
namespace engine {
Entity::Entity(entt::entity handle, entt::registry *registry)
    : m_handle(handle), m_registry(registry) {}
} // namespace engine