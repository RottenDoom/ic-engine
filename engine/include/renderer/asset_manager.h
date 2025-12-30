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
class ModelManager
{
public:
        ModelManager();

        ModelHandle loadModel(const char* path);
        void draw(ModelHandle handle);

private:
        std::unordered_map<const char*, ic::IModelLoader*> m_loaders;
        std::vector<ModelHandle> m_models;

        /** TODO: UUID */
        ModelHandle getNewHandle() { return ++m_HandleCount; };

        void registerLoader(ic::IModelLoader* modelLoader);
        size_t m_HandleCount;
};

class AssetManager
{
public:
        ModelManager modelManager{};
        ShaderManager shaderManager{};

        AssetManager();
        ~AssetManager();

        static AssetManager* Get();
        static void destroyInstance();

private:
        static AssetManager* s_instance;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /**
         * @function loadModelFromFile
         * @category render
         * @brief Loads model from a path with translate, scale and rotation.
         * @param const char * - path: Path to the file relative to the path or CWD_PATH
         * @param float* translate: Translate location for translating the model
         * @param float* scale: Scale transform for scaling the model
         * @param float* rotation: Rotation value for model rotation.
         * TODO: This API for now can only load static meshes I have got to research about dynamic meshes.
         * TODO: Instead of const char* path I am probably goint to use my own file system.
         * TODO: Instead of float pointers use my own math library at some point probably gonna use glm internally tho.
         */
        ModelHandle loadModelFromFile(const char* path, float* translate, float* scale, float* rotation);

        /**
         * @function unLoadModel
         * @category render
         * @brief UnLoads and frees the rescources of a model.
         * @param ModelHandle ID of the model to be released.
         */
        void unLoadModel(ModelHandle modelId);

        /**
         * @function drawModel
         * @category render
         * @brief Draws the model/scene to the screen. Use it in the call to render function
         * @param ModelHandle ID of the model to be drawn
         * @example > A simple example for drawing models
         * 	#include <ic_engine.h>
         *
         * 	ModelHandle scene = loadModel("CWD_PATH/assets/scene.gltf");
         *
         * 	void render() {
         * 		drawModel(scene);
         *      }
         */
        void drawModel(ModelHandle modelId);

#ifdef __cplusplus
}
#endif
