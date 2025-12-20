#include "gltf_loader.h"

#include <filesystem>
#include <string>
#include <variant>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define toIndex(x) static_cast<ic::Index>(x)

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
        for (auto& it : gltf->asset.scenes)
        {
                loadScene(gltf, it);
        }

        for (auto& it : gltf->asset.nodes)
        {
                loadNode(gltf, it);
        }

        for (auto& it : gltf->asset.meshes)
        {
                loadMesh(gltf, it);
        }

        for (auto& it : gltf->asset.samplers)
        {
                loadSamplers(gltf, it);
        }

        for (auto& it : gltf->asset.images)
        {
                loadImage(gltf, it);
        }

        for (auto& it : gltf->asset.textures)
        {
                loadTexture(gltf, it);
        }

        IC_CORE_TRACE("Loaded {} Scenes", gltf->scenes.size());
        IC_CORE_TRACE("Loaded {} Nodes", gltf->nodes.size());
        IC_CORE_TRACE("Loaded {} Meshes", gltf->meshes.size());
        // IC_CORE_TRACE("Loaded {} Images", gltf->images.size());

        /** TODO: handle this better */
        if (asset->defaultScene.has_value())
        {
                gltf->defaultScene = static_cast<ic::Index>(asset->defaultScene.value());
        }
        else if (!gltf->scenes.empty())
        {
                gltf->defaultScene = 0;
        }
        else
        {
                gltf->defaultScene = ic::INVALID_INDEX;
        }

        return true;
}

bool ic::GLTFLoader::loadScene(GLTFModel* gltf, fastgltf::Scene& scene)
{
        ic::Scene engineScene{};
        engineScene.name = scene.name;
        engineScene.rootNodes.reserve(scene.nodeIndices.size());

        for (size_t nodeIndex : scene.nodeIndices)
        {
                engineScene.rootNodes.push_back(static_cast<ic::Index>(nodeIndex));
        }

        gltf->scenes.push_back(std::move(engineScene));
        return true;
}

bool ic::GLTFLoader::loadNode(GLTFModel* gltf, fastgltf::Node& node)
{
        ic::Node gltfNode{};
        gltfNode.name = node.name; /** Node can also contain no name handle that too */
        if (node.meshIndex.has_value())
        {
                gltfNode.mesh = node.meshIndex.value();
        }
        if (node.skinIndex.has_value())
        {
                gltfNode.skin = node.skinIndex.value();
        }
        if (node.lightIndex.has_value())
        {
                gltfNode.light = node.lightIndex.value();
        }
        if (node.cameraIndex.has_value())
        {
                gltfNode.camera = node.cameraIndex.value();
        }

        gltfNode.children.reserve(node.children.size());
        for (auto& childIndex : node.children)
        {
                gltfNode.children.push_back(static_cast<ic::Index>(childIndex));
        }

        if (std::holds_alternative<fastgltf::TRS>(node.transform))
        {
                const auto& gltfTRS     = std::get<fastgltf::TRS>(node.transform);

                gltfNode.translation    = glm::vec3(gltfTRS.translation[0],
                                                 gltfTRS.translation[1],
                                                 gltfTRS.translation[2]);

                gltfNode.rotation       = glm::quat(gltfTRS.rotation[3],  // w component first in GLM
                                              gltfTRS.rotation[0],  // x
                                              gltfTRS.rotation[1],  // y
                                              gltfTRS.rotation[2]   // z
                );

                gltfNode.scale          = glm::vec3(gltfTRS.scale[0], gltfTRS.scale[1], gltfTRS.scale[2]);

                glm::mat4 matrix        = glm::mat4(1.0f);
                matrix                  = glm::translate(matrix, gltfNode.translation);
                matrix                  = matrix * glm::mat4_cast(gltfNode.rotation);
                matrix                  = glm::scale(matrix, gltfNode.scale);
                gltfNode.localTransform = matrix;
        }
        else if (std::holds_alternative<fastgltf::math::fmat4x4>(node.transform))
        {
                const auto& gltfMatrix = std::get<fastgltf::math::fmat4x4>(node.transform);

                // fastgltf stores column-major, GLM also uses column-major
                glm::mat4 matrix;
                memcpy(&matrix[0][0], gltfMatrix.data(), 16 * sizeof(float));

                gltfNode.localTransform = matrix;
        }
        gltf->nodes.push_back(std::move(gltfNode));
        return true;
}

bool ic::GLTFLoader::loadMesh(GLTFModel* gltf, fastgltf::Mesh& mesh)
{
        fastgltf::Asset& asset = gltf->asset;
        ic::Mesh outMesh{};
        outMesh.meshPrimitives.reserve(mesh.primitives.size());
        outMesh.name = mesh.name;

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it)
        {
                /** Each mesh contains primitives what indices it points to and material and mode.
                 *  The mesh has modes triangle, point or line, Also each primitive of a mesh has
                 *  Positions. We also initialize all the necessary data for the indices
                 *  We could have directly used the data to initialize the vertex arrays but that will require
                 *  me to write two different functions for the same thing but for different structs. I would rather
                 * write those functions in there respective structs
                 */
                ic::MeshPrimitive primitive;

                /** Sanity Checks */
                auto positionIt = it->findAttribute("POSITION");
                IC_CORE_ASSERT(positionIt != it->attributes.end(),
                               "Primitive Error: POSITION attribute is required for construct vertex arrays! Mesh {}",
                               mesh.name);

                IC_CORE_ASSERT(it->indicesAccessor.has_value(),
                               "Primitive Error: Index accessor not available for the mesh: {}",
                               mesh.name);

                // mode
                primitive.mode = static_cast<ic::MeshPrimitive::Mode>(it->type);

                // attributes
                for (const auto& [name, accessor] : it->attributes)
                {
                        if (name == "POSITION")
                                primitive.position = toIndex(accessor);
                        else if (name == "NORMAL")
                                primitive.normal = toIndex(accessor);
                        else if (name == "TANGENT")
                                primitive.tangent = toIndex(accessor);
                        else if (name == "COLOR_0")
                                primitive.color = toIndex(accessor);
                        else if (name.rfind("TEXCOORD_", 0) == 0)
                        {
                                // Extract the number after TEXCOORD_
                                /** TODO: make my own string class? */
                                size_t uvIndex = static_cast<size_t>(name[9] - '0');
                                if (primitive.texcoords.size() <= uvIndex)
                                        primitive.texcoords.resize(uvIndex + 1, INVALID_INDEX);

                                primitive.texcoords[uvIndex] = toIndex(accessor);
                        }
                        else if (name == "JOINTS_0")
                                primitive.joints = toIndex(accessor);
                        else if (name == "WEIGHTS_0")
                                primitive.weights = toIndex(accessor);
                }

                // indices
                if (it->indicesAccessor.has_value())
                        primitive.indices = toIndex(it->indicesAccessor.value());

                // material
                if (it->materialIndex.has_value())
                        primitive.material = toIndex(it->materialIndex.value());

                // it->target /** TODO: Morph Targets */
                // it-> dracoCompression /** TODO: Draco Compression using KTX? */

                outMesh.meshPrimitives.push_back(std::move(primitive));
        }
        gltf->meshes.push_back(std::move(outMesh));
        return true;
}

bool ic::GLTFLoader::loadSamplers(GLTFModel* gltf, fastgltf::Sampler& sampler)
{
        ic::Sampler tsampler{};
        tsampler.magFilter = static_cast<Sampler::Filter>(sampler.magFilter.value());
        tsampler.minFilter = static_cast<Sampler::Filter>(sampler.minFilter.value());
        tsampler.wrapS     = static_cast<Sampler::Wrap>(sampler.wrapS);
        tsampler.wrapT     = static_cast<Sampler::Wrap>(sampler.wrapT);
        gltf->samplers.push_back(std::move(tsampler));
        return true;
}

bool ic::GLTFLoader::loadMaterial(GLTFModel* gltf, fastgltf::Material& material)
{
        ic::Material mat;
        mat.name = material.name;

        /** PBR Data */
        auto& bCF = material.pbrData.baseColorFactor;

        /** TODO: better way of doing this */
        mat.pbr.baseColorFactor = glm::vec4(bCF[0], bCF[1], bCF[2], bCF[3]);
        mat.pbr.metallicFactor  = material.pbrData.metallicFactor;
        mat.pbr.roughnessFactor = material.pbrData.roughnessFactor;

        if (material.pbrData.baseColorTexture.has_value())
        {
                /** TODO: Texture transform */
                mat.pbr.baseColorTextureIndex = material.pbrData.baseColorTexture.value().textureIndex;
                mat.pbr.baseColorTextureCoord = material.pbrData.baseColorTexture.value().texCoordIndex;
        }

        if (material.pbrData.metallicRoughnessTexture.has_value())
        {
                mat.pbr.metallicRoughnessTextureIndex = material.pbrData.metallicRoughnessTexture.value().textureIndex;
                mat.pbr.metallicRoughnessTextureCoord = material.pbrData.metallicRoughnessTexture.value().texCoordIndex;
        }

        mat.alphaCutoff = material.alphaCutoff;
        mat.alphaMode   = static_cast<Material::AlphaMode>(material.alphaMode);

        /** Emission */
        auto& emm          = material.emissiveFactor;
        mat.emissiveFactor = glm::vec3(emm[0], emm[1], emm[2]);

        if (material.emissiveTexture.has_value())
        {
                mat.emissive.index    = material.emissiveTexture.value().textureIndex;
                mat.emissive.texCoord = material.emissiveTexture.value().texCoordIndex;
        }

        /** Normal */
        if (material.normalTexture.has_value())
        {
                mat.normal.index    = material.normalTexture.value().textureIndex;
                mat.normal.texCoord = material.normalTexture.value().texCoordIndex;
                mat.normal.scale    = material.normalTexture.value().scale;
        }

        /** Occlusion */
        if (material.occlusionTexture.has_value())
        {
                mat.occlusion.index    = material.occlusionTexture.value().textureIndex;
                mat.occlusion.texCoord = material.occlusionTexture.value().texCoordIndex;
                mat.occlusion.strength = material.occlusionTexture.value().strength;
        }

        gltf->materials.push_back(std::move(mat));
        return false;
}

bool ic::GLTFLoader::loadImage(GLTFModel* gltf, fastgltf::Image& image)
{
        ic::ImageData imageData;

        /** TODO: Gotta remove the std::variant things */
        std::visit(
            fastgltf::visitor{
                [](auto& arg) {},
                [&](fastgltf::sources::URI& filePath)
                {
                        IC_CORE_ASSERT(filePath.fileByteOffset == 0, "STBI Offset error!");
                        IC_CORE_ASSERT(filePath.uri.isLocalPath(), "Did not find the URI path");
                        int width, height, nrChannels;

                        const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                        uint8_t* data      = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                        imageData.channels = nrChannels;
                        imageData.width    = width;
                        imageData.height   = height;

                        size_t size        = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
                        imageData.pixels.assign(data, data + size);
                        stbi_image_free(data);
                },
                [&](fastgltf::sources::Array& vector)
                {
                        int width, height, nrChannels;
                        uint8_t* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
                                                              static_cast<int>(vector.bytes.size()),
                                                              &width,
                                                              &height,
                                                              &nrChannels,
                                                              4);
                        imageData.channels = nrChannels;
                        imageData.width    = width;
                        imageData.height   = height;

                        size_t size        = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
                        imageData.pixels.assign(data, data + size);
                        stbi_image_free(data);
                },
                [&](fastgltf::sources::BufferView& view)
                {
                        auto& bufferView = gltf->asset.bufferViews[view.bufferViewIndex];
                        auto& buffer     = gltf->asset.buffers[bufferView.bufferIndex];
                        std::visit(fastgltf::visitor{[](auto& arg) {},
                                                     [&](fastgltf::sources::Array& vector)
                                                     {
                                                             int width, height, nrChannels;
                                                             unsigned char* data = stbi_load_from_memory(
                                                                 reinterpret_cast<const stbi_uc*>(
                                                                     vector.bytes.data() + bufferView.byteOffset),
                                                                 static_cast<int>(bufferView.byteLength),
                                                                 &width,
                                                                 &height,
                                                                 &nrChannels,
                                                                 4);

                                                             imageData.channels = nrChannels;
                                                             imageData.width    = width;
                                                             imageData.height   = height;

                                                             size_t size        = static_cast<size_t>(width) *
                                                                           static_cast<size_t>(height) * 4;
                                                             imageData.pixels.assign(data, data + size);
                                                             stbi_image_free(data);
                                                     }},
                                   buffer.data);
                },
            },
            image.data);

        gltf->images.push_back(std::move(imageData));
        return true;
}

bool ic::GLTFLoader::loadTexture(GLTFModel* gltf, fastgltf::Texture& texture)
{
        ic::Texture tex;

        if (texture.imageIndex.has_value())
        {
                tex.image = toIndex(texture.imageIndex.value());
        }
        if (texture.samplerIndex.has_value())
        {
                tex.sampler = toIndex(texture.samplerIndex.value());
        }
        /** TODO: basisu or whatever */

        gltf->textures.push_back(std::move(tex));
}

bool ic::GLTFLoader::loadCamera(GLTFModel* gltf, fastgltf::Camera& camera)
{
        return false;
}
