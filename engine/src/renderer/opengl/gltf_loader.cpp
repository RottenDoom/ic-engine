#include "gltf_loader.h"

/** TODO: remove all this  */
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

#include <filesystem>

ic::GLTFLoader::~GLTFLoader() {}

bool ic::GLTFLoader::loadModel(const char* path, Model* model)
{
        GLTFModel* gltf = static_cast<GLTFModel*>(model);
        if (!gltf)
        {
                IC_CORE_WARN("Invalid model type passed to GLTFLoader");
                return false;
        }

        return loadGLTF(path, gltf);
}

bool ic::GLTFLoader::loadGLTF(std::filesystem::path path, GLTFModel* gltf)
{
        if (!std::filesystem::exists(path))
        {
                IC_CORE_WARN("Failed to find {}!", path.string());
                return false;
        }

        if constexpr (std::is_same_v<std::filesystem::path::value_type, wchar_t>)
        {
                IC_CORE_INFO("Loading {}", path.string());
        }
        else
        {
                IC_CORE_INFO("Loading {}", path.string());
        }

        static constexpr auto supportedExtensions = fastgltf::Extensions::KHR_mesh_quantization |
                                                    fastgltf::Extensions::KHR_texture_transform |
                                                    fastgltf::Extensions::KHR_materials_variants;

        fastgltf::Parser parser(supportedExtensions);

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                     fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages |
                                     fastgltf::Options::GenerateMeshIndices;

        auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
        if (!bool(gltfFile))
        {
                IC_CORE_WARN("Failed to open glTF file: {}", fastgltf::getErrorMessage(gltfFile.error()));
                return false;
        }

        auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
        if (asset.error() != fastgltf::Error::None)
        {
                IC_CORE_WARN("Failed to load glTF: {}\nDirectory: {}",
                             fastgltf::getErrorMessage(asset.error()),
                             path.parent_path().generic_string());
                return false;
        }

        gltf->asset = std::move(asset.get());
        IC_CORE_INFO("Models Loaded: {}", gltf->asset.meshes.size());
        return true;
}

bool ic::GLTFLoader::loadMesh(GLTFModel* gltf, fastgltf::Mesh& mesh)
{
        fastgltf::Asset& asset = gltf->asset;
        ic::Mesh outMesh{};
        outMesh.meshPrimitives.resize(mesh.primitives.size());

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); it++)
        {
                auto positionIt = it->findAttribute("POSITION");

                IC_CORE_ASSERT(positionIt != it->attributes.end(),
                               "No Position vertices given in the GLTF model!");  // A mesh primitive is required to
                                                                                  // hold the POSITION attribute.
                IC_CORE_ASSERT(it->indicesAccessor.has_value(), "Mesh does not have index accessor");
        }

        return true;
}
