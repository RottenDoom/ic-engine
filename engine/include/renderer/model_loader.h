#pragma once
#include "../defines.h"

/** TODO: Create a model type? */

using ModelHandle = int64_t;
#define INVALID_MODEL_HANDLE -1

namespace ic
{
struct Model;

/**
 * @brief IModelLoader is a base class for any type of model loader. Currently GLTF and GLB models can be loaded easily
 * More to come
 */
class IModelLoader
{
public:
        typedef enum
        {
                NONE,
                GLTF,
                OBJ,
                FBX
        } ModelType;
        ModelType type                                         = NONE;
        virtual bool loadModel(const char* path, Model* model) = 0;
        virtual ~IModelLoader() {}
};

/**
 * @brief ShaderManager will be the global class to hold all the shader handles. I want to make an API in future for
 * users to write their own shaders but I will do that later.
 */
class ShaderManager
{
public:
        ShaderManager() = default;
};

/** @brief Modelmanager is another class of AssetManager that will load and draw the models and hold the models. This
 * class hopefully adds Models to models vector which can be manipulated.
 */

}  // namespace ic

// TODOS:

/**
 * 1. Asset conditioning pipeline. Check fastgltf
 * 2. Handle different toolchain files
 * 3. GUIDs and cross references. Not really required in GLTF. but I will model my own game and thus requires me to
 * cross reference my game objects
 * 4. Global resource manager. Loading/Unloading -
 * 5. Reference counting resources. Not sure if it will be implemented since I am creating only one level
 *
 *
 * So basically I am to create a basic scaffolding of something that will understand my own game data types and formats
 * Something that acts as a asset manager for now. I will start creating multi model scenes and eventually load an
 * animation as well. Goal: using asset manager load a scene with animations a world and a skybox. Time Limit - 2 weeks.
 * Else fuck off. Just write things bruh
 */