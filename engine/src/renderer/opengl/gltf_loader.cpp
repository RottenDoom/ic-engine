#include "renderer/opengl/gltf_loader.h"
#include "renderer/opengl/gl_model.h"
#include "renderer/opengl/gl_material.h"

#include <filesystem>
#include <string>
#include <variant>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ic
{
#define toIndex(x) static_cast<ic::Index>(x)

static GLenum toGLenum(fastgltf::ComponentType type)
{
        return static_cast<GLenum>(static_cast<uint16_t>(type) & 0x1FFF);
}

GLTFLoader::~GLTFLoader() {}

bool GLTFLoader::loadModel(const char* path, Model* model)
{
        if (!model)
        {
                IC_CORE_WARN("Invalid model type passed to GLTFLoader");
                return false;
        }

        return loadGLTF(path, model);
}

bool GLTFLoader::loadGLTF(std::filesystem::path path, Model* gltf)
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

        /** Sounds like some finance term */
        auto expectedAsset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
        if (expectedAsset.error() != fastgltf::Error::None)
        {
                IC_CORE_WARN("Failed to load glTF: {}\nDirectory: {}",
                             fastgltf::getErrorMessage(expectedAsset.error()),
                             path.parent_path().generic_string());
                return false;
        }

        fastgltf::Asset* asset;
        if (const auto assetPtr = expectedAsset.get_if())
        {

                asset = assetPtr;
        }

        for (auto& it : asset->scenes)
        {
                loadScene(gltf, it);
        }

        for (auto& it : asset->nodes)
        {
                loadNode(gltf, it);
        }

        for (auto& it : asset->meshes)
        {
                loadMesh(gltf, it);
        }

        for (auto& it : asset->samplers)
        {
                loadSamplers(gltf, it);
        }

        for (auto& it : asset->materials)
        {
                loadMaterial(gltf, it);
        }

        for (auto& it : asset->images)
        {
                loadImage(gltf, *asset, it);
        }

        for (auto& it : asset->textures)
        {
                loadTexture(gltf, it);
        }

        for (auto& it : asset->accessors)
        {
                loadAccessor(gltf, it);
        }

        for (auto& it : asset->bufferViews)
        {
                loadBufferView(gltf, it);
        }

        for (auto& it : asset->buffers)
        {
                loadBuffer(gltf, it, path); /** See if this path is correct */
        }

        /** TODO: remove these */
        IC_CORE_TRACE("Loaded {} Scenes", gltf->scenes.size());
        IC_CORE_TRACE("Loaded {} Nodes", gltf->nodes.size());
        IC_CORE_TRACE("Loaded {} Meshes", gltf->meshes.size());
        IC_CORE_TRACE("Loaded {} Images", gltf->images.size());
        IC_CORE_TRACE("Loaded {} Textures", gltf->textures.size());
        IC_CORE_TRACE("Loaded {} Buffers", gltf->buffers.size());
        IC_CORE_TRACE("Loaded {} bufferViews", gltf->bufferViews.size());
        IC_CORE_TRACE("Loaded {} accessors", gltf->accessors.size());
        IC_CORE_TRACE("Loaded {} materials", gltf->materials.size());

        /** TODO: handle this better */
        if (asset->defaultScene.has_value())
        {
                gltf->defaultScene = toIndex(asset->defaultScene.value());
        }
        else if (!gltf->scenes.empty())
        {
                gltf->defaultScene = 0;
        }
        else
        {
                gltf->defaultScene = INVALID_INDEX;
        }

        return true;
}

bool GLTFLoader::loadScene(Model* gltf, fastgltf::Scene& scene)
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

bool GLTFLoader::loadNode(Model* gltf, fastgltf::Node& node)
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

bool GLTFLoader::loadMesh(Model* gltf, fastgltf::Mesh& mesh)
{
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

bool GLTFLoader::loadSamplers(Model* gltf, fastgltf::Sampler& sampler)
{
        ic::Sampler tsampler{};
        tsampler.magFilter = static_cast<Sampler::Filter>(sampler.magFilter.value());
        tsampler.minFilter = static_cast<Sampler::Filter>(sampler.minFilter.value());
        tsampler.wrapS     = static_cast<Sampler::Wrap>(sampler.wrapS);
        tsampler.wrapT     = static_cast<Sampler::Wrap>(sampler.wrapT);
        gltf->samplers.push_back(std::move(tsampler));
        return true;
}

bool GLTFLoader::loadMaterial(Model* gltf, fastgltf::Material& material)
{
        ic::Material mat;
        mat.name = material.name;

        /** PBR Data */
        auto& bCF                       = material.pbrData.baseColorFactor;

        mat.pbrMaterial.baseColorFactor = glm::vec4(bCF[0], bCF[1], bCF[2], bCF[3]);
        mat.pbrMaterial.metallicFactor  = material.pbrData.metallicFactor;
        mat.pbrMaterial.roughnessFactor = material.pbrData.roughnessFactor;

        if (material.pbrData.baseColorTexture.has_value())
        {
                /** TODO: Texture transform */
                mat.pbrMaterial.baseColorTexture.textureInfo.idx =
                    material.pbrData.baseColorTexture.value().textureIndex;
                mat.pbrMaterial.baseColorTexture.textureInfo.texCoord =
                    material.pbrData.baseColorTexture.value().texCoordIndex;
        }

        if (material.pbrData.metallicRoughnessTexture.has_value())
        {
                mat.pbrMaterial.metallicRoughnessTexture.textureInfo.idx =
                    material.pbrData.metallicRoughnessTexture.value().textureIndex;
                mat.pbrMaterial.metallicRoughnessTexture.textureInfo.texCoord =
                    material.pbrData.metallicRoughnessTexture.value().texCoordIndex;
        }

        mat.alphaCutoff = material.alphaCutoff;
        mat.alphaMode   = static_cast<Material::AlphaMode>(material.alphaMode);

        /** Emission */
        auto& emm          = material.emissiveFactor;
        mat.emissiveFactor = glm::vec3(emm[0], emm[1], emm[2]);

        /** Normal */
        if (material.normalTexture.has_value())
        {
                mat.normalTexture.textureInfo.idx      = material.normalTexture.value().textureIndex;
                mat.normalTexture.textureInfo.texCoord = material.normalTexture.value().texCoordIndex;
                mat.normalTexture.scale                = material.normalTexture.value().scale;
        }

        /** Occlusion */
        if (material.occlusionTexture.has_value())
        {
                mat.occlusionTexture.textureInfo.idx      = material.occlusionTexture.value().textureIndex;
                mat.occlusionTexture.textureInfo.texCoord = material.occlusionTexture.value().texCoordIndex;
                mat.occlusionTexture.strength             = material.occlusionTexture.value().strength;
        }

        if (material.emissiveTexture.has_value())
        {
                mat.emissiveTexture.textureInfo.idx      = material.emissiveTexture.value().textureIndex;
                mat.emissiveTexture.textureInfo.texCoord = material.emissiveTexture.value().texCoordIndex;
        }

        gltf->materials.push_back(std::move(mat));
        return true;
}

Accessor::Type GLTFLoader::convertAccessorType(fastgltf::AccessorType type)
{

        switch (type)
        {
        case fastgltf::AccessorType::Scalar:
                return Accessor::Type::SCALAR;
        case fastgltf::AccessorType::Vec2:
                return Accessor::Type::VEC2;
        case fastgltf::AccessorType::Vec3:
                return Accessor::Type::VEC3;
        case fastgltf::AccessorType::Vec4:
                return Accessor::Type::VEC4;
        case fastgltf::AccessorType::Mat4:
                return Accessor::Type::MAT4;
        default:
                return Accessor::Type::UNKNOWN;
        }
}

void GLTFLoader::loadBufferView(Model* gltf, fastgltf::BufferView& bufferView)
{
        ic::BufferView bufView;

        bufView.bufferIndex = bufferView.bufferIndex;
        bufView.byteOffset  = bufferView.byteOffset;
        bufView.byteLength  = bufferView.byteLength;
        bufView.byteStride  = bufferView.byteStride.value_or(0);

        bufView.name        = bufferView.name;

        gltf->bufferViews.push_back(std::move(bufView));
}

void GLTFLoader::loadBuffer(Model* gltf, const fastgltf::Buffer& buffer, const std::filesystem::path& basePath)
{
        ic::Buffer buf;
        std::visit(fastgltf::visitor{[&](const fastgltf::sources::Array& array)
                                     {
                                             buf.data.resize(array.bytes.size());
                                             memcpy(buf.data.data(), array.bytes.data(), array.bytes.size());
                                     },
                                     [&](const fastgltf::sources::Vector& vector)
                                     {
                                             buf.data.resize(vector.bytes.size());
                                             memcpy(buf.data.data(), vector.bytes.data(), vector.bytes.size());
                                     },
                                     [&](const fastgltf::sources::ByteView& view)
                                     {
                                             buf.data.resize(view.bytes.size());
                                             memcpy(buf.data.data(), view.bytes.data(), view.bytes.size());
                                     },
                                     [&](const fastgltf::sources::URI& uri)
                                     {
                                             // External file - need to load it
                                             std::filesystem::path bufferPath = basePath / uri.uri.path();

                                             std::ifstream file(bufferPath, std::ios::binary | std::ios::ate);
                                             if (!file)
                                             {
                                                     IC_CORE_ERROR("Failed to open buffer file: {}",
                                                                   bufferPath.string());
                                                     return;
                                             }

                                             size_t fileSize = file.tellg();
                                             file.seekg(0);

                                             buf.data.resize(fileSize);
                                             file.read(reinterpret_cast<char*>(buf.data.data()), fileSize);
                                     },
                                     [&](auto&& arg) { IC_CORE_WARN("Unsupported buffer source type"); }},
                   buffer.data);
        gltf->buffers.push_back(std::move(buf));
}

void GLTFLoader::loadAccessor(Model* gltf, fastgltf::Accessor& accessor)
{
        ic::Accessor acc;
        acc.bufferView      = accessor.bufferViewIndex.has_value() ? toIndex(accessor.bufferViewIndex.value())
                                                                   : INVALID_INDEX;
        acc.offset          = accessor.byteOffset;
        acc.count           = accessor.count;
        acc.type            = convertAccessorType(accessor.type);

        acc.componentType   = toGLenum(accessor.componentType);
        acc.normalized      = accessor.normalized;

        uint32_t components = getAccessorComponentCount(acc.type);

        // Extract min/max values from AccessorBoundsArray
        if (accessor.min.has_value())
        {
                const auto& minArray = accessor.min.value();
                acc.min.resize(minArray.size());

                // Check which type the bounds array holds and extract accordingly
                if (minArray.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
                {
                        const double* data = minArray.data<double>();
                        for (size_t i = 0; i < minArray.size(); ++i)
                        {
                                acc.min[i] = data[i];
                        }
                }
                else if (minArray.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
                {
                        const int64_t* data = minArray.data<int64_t>();
                        for (size_t i = 0; i < minArray.size(); ++i)
                        {
                                acc.min[i] = static_cast<double>(data[i]);
                        }
                }
        }

        if (accessor.max.has_value())
        {
                const auto& maxArray = accessor.max.value();
                acc.max.resize(maxArray.size());

                if (maxArray.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
                {
                        const double* data = maxArray.data<double>();
                        for (size_t i = 0; i < maxArray.size(); ++i)
                        {
                                acc.max[i] = data[i];
                        }
                }
                else if (maxArray.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
                {
                        const int64_t* data = maxArray.data<int64_t>();
                        for (size_t i = 0; i < maxArray.size(); ++i)
                        {
                                acc.max[i] = static_cast<double>(data[i]);
                        }
                }
        }

        gltf->accessors.push_back(std::move(acc));
}

bool GLTFLoader::loadImage(Model* gltf, fastgltf::Asset& asset, fastgltf::Image& image)
{
        ic::ImageData imageData;

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
                        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                        auto& buffer     = asset.buffers[bufferView.bufferIndex];
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

void GLTFLoader::loadTexture(Model* gltf, fastgltf::Texture& texture)
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

bool GLTFLoader::loadCamera(Model* gltf, fastgltf::Camera& camera)
{
        return false;
}
}  // namespace ic
