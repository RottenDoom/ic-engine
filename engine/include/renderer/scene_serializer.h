#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H

#include "defines.h"
#include "scene.h"

namespace ic
{
class IC_API SceneSerializer {
public:
	void Serialize(const char* path, ic::RenderScene* scene);
	ic::RenderScene* Deserialize(const char* path);
};

} // namespace ic


#endif // SCENE_SERIALIZER_H