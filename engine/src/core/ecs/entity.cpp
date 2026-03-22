#include "core/ecs/entity.h"

namespace ic
{

Entity::Entity(entt::entity handle, RenderScene *scene) : handle(handle), scene(scene) {}

}  // namespace ic