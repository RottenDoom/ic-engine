#include "core/gltf_loader.h"
#include "renderer/opengl/gl_model.h"
#include "renderer/opengl/gl_material.h"

#include <filesystem>
#include <string>
#include <variant>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ic
{
#define toIndex(x) static_cast<Index>(x)

static GLenum toGLenum(fastgltf::ComponentType type);
static size_t getAccessorComponentCount(Accessor::Type type);

GLTFLoader::~GLTFLoader() {}

bool GLTFLoader::loadModel(const char *path, Model *model)
{
        if (!model)
        {
                IC_CORE_WARN("Invalid model type passed to GLTFLoader");
                return false;
        }

        return loadGLTF(path, model);
}

bool GLTFLoader::loadGLTF(std::filesystem::path path, Model *gltf)
{
        if (!std::filesystem::exists(path))
        {
                IC_CORE_WARN("Failed to find {}!", path.string());
                return false;
        }

        IC_CORE_INFO("Loading {}", path.string());

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

        auto expectedAsset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
        if (expectedAsset.error() != fastgltf::Error::None)
        {
                IC_CORE_WARN("Failed to load glTF: {}\nDirectory: {}",
                             fastgltf::getErrorMessage(expectedAsset.error()),
                             path.parent_path().generic_string());
                return false;
        }

        fastgltf::Asset *asset = expectedAsset.get_if();
        if (!asset)
        {
                IC_CORE_WARN("Failed to get asset from GLTF file");
                return false;
        }

        // Load raw GLTF data first (buffers, accessors, etc.)
        for (auto &it : asset->buffers)
        {
                loadBuffer(gltf, it, path);
        }

        for (auto &it : asset->bufferViews)
        {
                loadBufferView(gltf, it);
        }

        for (auto &it : asset->accessors)
        {
                loadAccessor(gltf, it);
        }

        // Load resources
        for (auto &it : asset->samplers)
        {
                loadSamplers(gltf, it);
        }

        for (auto &it : asset->images)
        {
                loadImage(gltf, *asset, it);
        }

        for (auto &it : asset->textures)
        {
                loadTexture(gltf, it);
        }

        for (auto &it : asset->materials)
        {
                loadMaterial(gltf, it);
        }

        // Load scene graph
        for (auto &it : asset->nodes)
        {
                loadNode(gltf, it);
        }

        // Load meshes and process geometry
        for (auto &it : asset->meshes)
        {
                loadMesh(gltf, it);
        }

        for (auto &it : asset->scenes)
        {
                loadScene(gltf, it);
        }

        IC_CORE_TRACE("Loaded {} Scenes", gltf->scenes.size());
        IC_CORE_TRACE("Loaded {} Nodes", gltf->nodes.size());
        IC_CORE_TRACE("Loaded {} Meshes", gltf->meshes.size());
        IC_CORE_TRACE("Loaded {} Images", gltf->images.size());
        IC_CORE_TRACE("Loaded {} Textures", gltf->textures.size());
        IC_CORE_TRACE("Loaded {} Buffers", gltf->buffers.size());
        IC_CORE_TRACE("Loaded {} BufferViews", gltf->bufferViews.size());
        IC_CORE_TRACE("Loaded {} Accessors", gltf->accessors.size());
        IC_CORE_TRACE("Loaded {} Materials", gltf->materials.size());

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

        // Process meshes to extract vertex/index data
        processMeshGeometry(gltf);

        // Can free loading data now if needed
        // gltf->FreeLoadingData();

        return true;
}

bool GLTFLoader::loadScene(Model *gltf, fastgltf::Scene &scene)
{
        Scene engineScene{};
        engineScene.name = scene.name;
        engineScene.rootNodes.reserve(scene.nodeIndices.size());

        for (size_t nodeIndex : scene.nodeIndices)
        {
                engineScene.rootNodes.push_back(static_cast<Index>(nodeIndex));
        }

        gltf->scenes.push_back(std::move(engineScene));
        return true;
}

bool GLTFLoader::loadNode(Model *gltf, fastgltf::Node &node)
{
        Node gltfNode{};
        gltfNode.name = node.name; /** Node can also contain no name handle that too */
        if (node.meshIndex.has_value())
        {
                gltfNode.meshIndex = node.meshIndex.value();
        }
        if (node.skinIndex.has_value())
        {
                gltfNode.skinIndex = node.skinIndex.value();
        }
        if (node.lightIndex.has_value())
        {
                gltfNode.lightIndex = node.lightIndex.value();
        }
        if (node.cameraIndex.has_value())
        {
                gltfNode.cameraIndex = node.cameraIndex.value();
        }

        gltfNode.children.reserve(node.children.size());
        for (auto &childIndex : node.children)
        {
                gltfNode.children.push_back(static_cast<Index>(childIndex));
        }

        if (std::holds_alternative<fastgltf::TRS>(node.transform))
        {
                const auto &gltfTRS     = std::get<fastgltf::TRS>(node.transform);

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
                const auto &gltfMatrix = std::get<fastgltf::math::fmat4x4>(node.transform);

                // fastgltf stores column-major, GLM also uses column-major
                glm::mat4 matrix;
                memcpy(&matrix[0][0], gltfMatrix.data(), 16 * sizeof(float));

                gltfNode.localTransform = matrix;
        }
        gltf->nodes.push_back(std::move(gltfNode));
        return true;
}

bool GLTFLoader::loadMesh(Model *gltf, fastgltf::Mesh &mesh)
{
        Mesh outMesh{};
        outMesh.primitives.reserve(mesh.primitives.size());
        outMesh.name = mesh.name;

        /** Each mesh contains primitives what indices it points to and material and mode.
         *  The mesh has modes triangle, point or line, Also each primitive of a mesh has
         *  Positions. We also initialize all the necessary data for the indices
         *  We could have directly used the data to initialize the vertex arrays but that will require
         *  me to write two different functions for the same thing but for different structs. I would
         * rather write those functions in there respective structs
         */
        for (auto &primitive : mesh.primitives)
        {
                MeshPrimitive prim;
                prim.mode = static_cast<MeshPrimitive::Mode>(primitive.type);

                // TODO: Store accessor indices temporarily (will be processed later)
                TempPrimitiveData tempData;

                auto positionIt = primitive.findAttribute("POSITION");
                IC_CORE_ASSERT(positionIt != primitive.attributes.end(),
                               "POSITION attribute required for mesh: {}",
                               mesh.name);
                tempData.positionAccessor = toIndex(positionIt->accessorIndex);

                for (const auto &[name, accessorIndex] : primitive.attributes)
                {
                        if (name == "NORMAL")
                                tempData.normalAccessor = toIndex(accessorIndex);
                        else if (name == "TANGENT")
                                tempData.tangentAccessor = toIndex(accessorIndex);
                        else if (name == "COLOR_0")
                                tempData.colorAccessor = toIndex(accessorIndex);
                        else if (name == "TEXCOORD_0")
                                tempData.texCoord0Accessor = toIndex(accessorIndex);
                        else if (name == "TEXCOORD_1")
                                tempData.texCoord1Accessor = toIndex(accessorIndex);
                        else if (name == "JOINTS_0")
                                tempData.jointsAccessor = toIndex(accessorIndex);
                        else if (name == "WEIGHTS_0")
                                tempData.weightsAccessor = toIndex(accessorIndex);
                }

                if (primitive.indicesAccessor.has_value())
                {
                        tempData.indicesAccessor = toIndex(primitive.indicesAccessor.value());
                }

                if (primitive.materialIndex.has_value())
                {
                        prim.materialIndex = toIndex(primitive.materialIndex.value());
                }

                // Store temp data for processing
                m_tempPrimitiveData.push_back(tempData);
                outMesh.primitives.push_back(std::move(prim));
        }

        gltf->meshes.push_back(std::move(outMesh));
        return true;
}

bool GLTFLoader::loadSamplers(Model *gltf, fastgltf::Sampler &sampler)
{
        Sampler tsampler{};
        if (sampler.magFilter.has_value())
                tsampler.magFilter = static_cast<Sampler::Filter>(sampler.magFilter.value());
        if (sampler.minFilter.has_value())
                tsampler.minFilter = static_cast<Sampler::Filter>(sampler.minFilter.value());
        tsampler.wrapS = static_cast<Sampler::Wrap>(sampler.wrapS);
        tsampler.wrapT = static_cast<Sampler::Wrap>(sampler.wrapT);
        gltf->samplers.push_back(std::move(tsampler));
        return true;
}

bool GLTFLoader::loadMaterial(Model *gltf, fastgltf::Material &material)
{
        Material mat;
        mat.name = material.name;

        /** PBR Data */
        auto &bCF                       = material.pbrData.baseColorFactor;

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
        auto &emm          = material.emissiveFactor;
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

void GLTFLoader::processMeshGeometry(Model *gltf)
{
        size_t primitiveIndex = 0;

        for (auto &mesh : gltf->meshes)
        {
                for (auto &primitive : mesh.primitives)
                {
                        if (primitiveIndex >= m_tempPrimitiveData.size())
                        {
                                IC_CORE_ERROR("Primitive index out of range during geometry processing");
                                continue;
                        }

                        const auto &tempData = m_tempPrimitiveData[primitiveIndex++];

                        // Extract vertices
                        extractVertices(gltf, tempData, primitive);

                        // Extract indices
                        extractIndices(gltf, tempData, primitive);
                }
        }

        // Clear temporary data
        m_tempPrimitiveData.clear();
}

void GLTFLoader::extractVertices(Model *gltf, const TempPrimitiveData &tempData, MeshPrimitive &primitive)
{
        if (tempData.positionAccessor == INVALID_INDEX)
                return;

        const Accessor &posAccessor = gltf->accessors[tempData.positionAccessor];
        size_t vertexCount          = posAccessor.count;

        primitive.vertices.resize(vertexCount);

        // Read positions
        std::vector<glm::vec3> positions;
        readAccessorVec3(gltf, tempData.positionAccessor, positions);

        for (size_t i = 0; i < vertexCount; ++i)
        {
                primitive.vertices[i].pos = positions[i];
        }

        // Read normals
        if (tempData.normalAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec3> normals;
                readAccessorVec3(gltf, tempData.normalAccessor, normals);
                for (size_t i = 0; i < std::min(vertexCount, normals.size()); ++i)
                {
                        primitive.vertices[i].normal = normals[i];
                }
        }

        // Read tangents
        if (tempData.tangentAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> tangents;
                readAccessorVec4(gltf, tempData.tangentAccessor, tangents);
                for (size_t i = 0; i < std::min(vertexCount, tangents.size()); ++i)
                {
                        primitive.vertices[i].tangent = tangents[i];
                }
        }

        // Read UVs
        if (tempData.texCoord0Accessor != INVALID_INDEX)
        {
                std::vector<glm::vec2> uvs;
                readAccessorVec2(gltf, tempData.texCoord0Accessor, uvs);
                for (size_t i = 0; i < std::min(vertexCount, uvs.size()); ++i)
                {
                        primitive.vertices[i].uv0 = uvs[i];
                }
        }

        if (tempData.texCoord1Accessor != INVALID_INDEX)
        {
                std::vector<glm::vec2> uvs;
                readAccessorVec2(gltf, tempData.texCoord1Accessor, uvs);
                for (size_t i = 0; i < std::min(vertexCount, uvs.size()); ++i)
                {
                        primitive.vertices[i].uv1 = uvs[i];
                }
        }

        // Read colors
        if (tempData.colorAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> colors;
                readAccessorVec4(gltf, tempData.colorAccessor, colors);
                for (size_t i = 0; i < std::min(vertexCount, colors.size()); ++i)
                {
                        primitive.vertices[i].color = colors[i];
                }
        }

        // Read skinning data
        if (tempData.jointsAccessor != INVALID_INDEX)
        {
                std::vector<glm::uvec4> joints;
                readAccessorUVec4(gltf, tempData.jointsAccessor, joints);
                for (size_t i = 0; i < std::min(vertexCount, joints.size()); ++i)
                {
                        primitive.vertices[i].joint0 = joints[i];
                }
        }

        if (tempData.weightsAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> weights;
                readAccessorVec4(gltf, tempData.weightsAccessor, weights);
                for (size_t i = 0; i < std::min(vertexCount, weights.size()); ++i)
                {
                        primitive.vertices[i].weight0 = weights[i];
                }
        }
}

void GLTFLoader::extractIndices(Model *gltf, const TempPrimitiveData &tempData, MeshPrimitive &primitive)
{
        if (tempData.indicesAccessor == INVALID_INDEX)
                return;

        const Accessor &idxAccessor = gltf->accessors[tempData.indicesAccessor];
        primitive.indices.resize(idxAccessor.count);

        // Indices can be different types, need to handle each
        const uint8_t *data = getAccessorData(gltf, tempData.indicesAccessor);

        switch (idxAccessor.componentType)
        {
        case Accessor::ComponentType::UByte:
        {
                const uint8_t *indices = reinterpret_cast<const uint8_t *>(data);
                for (size_t i = 0; i < idxAccessor.count; ++i)
                {
                        primitive.indices[i] = static_cast<uint32_t>(indices[i]);
                }
                break;
        }
        case Accessor::ComponentType::UShort:
        {
                const uint16_t *indices = reinterpret_cast<const uint16_t *>(data);
                for (size_t i = 0; i < idxAccessor.count; ++i)
                {
                        primitive.indices[i] = static_cast<uint32_t>(indices[i]);
                }
                break;
        }
        case Accessor::ComponentType::UInt:
        {
                const uint32_t *indices = reinterpret_cast<const uint32_t *>(data);
                memcpy(primitive.indices.data(), indices, idxAccessor.count * sizeof(uint32_t));
                break;
        }
        default:
                IC_CORE_ERROR("Unsupported index component type");
                break;
        }
}

const uint8_t *GLTFLoader::getAccessorData(Model *gltf, Index accessorIndex)
{
        if (accessorIndex >= gltf->accessors.size())
                return nullptr;

        const Accessor &accessor = gltf->accessors[accessorIndex];
        if (accessor.bufferView >= gltf->bufferViews.size())
                return nullptr;

        const BufferView &bufferView = gltf->bufferViews[accessor.bufferView];
        if (bufferView.bufferIndex >= gltf->buffers.size())
                return nullptr;

        const Buffer &buffer = gltf->buffers[bufferView.bufferIndex];

        return buffer.data.data() + bufferView.byteOffset + accessor.offset;
}

void GLTFLoader::readAccessorVec2(Model *gltf, Index accessorIndex, std::vector<glm::vec2> &outData)
{
        const Accessor &accessor = gltf->accessors[accessorIndex];
        outData.resize(accessor.count);

        const uint8_t *data    = getAccessorData(gltf, accessorIndex);
        const float *floatData = reinterpret_cast<const float *>(data);

        for (size_t i = 0; i < accessor.count; ++i)
        {
                outData[i] = glm::vec2(floatData[i * 2], floatData[i * 2 + 1]);
        }
}

void GLTFLoader::readAccessorVec3(Model *gltf, Index accessorIndex, std::vector<glm::vec3> &outData)
{
        const Accessor &accessor = gltf->accessors[accessorIndex];
        outData.resize(accessor.count);

        const uint8_t *data    = getAccessorData(gltf, accessorIndex);
        const float *floatData = reinterpret_cast<const float *>(data);

        for (size_t i = 0; i < accessor.count; ++i)
        {
                outData[i] = glm::vec3(floatData[i * 3], floatData[i * 3 + 1], floatData[i * 3 + 2]);
        }
}

void GLTFLoader::readAccessorVec4(Model *gltf, Index accessorIndex, std::vector<glm::vec4> &outData)
{
        const Accessor &accessor = gltf->accessors[accessorIndex];
        outData.resize(accessor.count);

        const uint8_t *data    = getAccessorData(gltf, accessorIndex);
        const float *floatData = reinterpret_cast<const float *>(data);

        for (size_t i = 0; i < accessor.count; ++i)
        {
                outData[i] =
                    glm::vec4(floatData[i * 4], floatData[i * 4 + 1], floatData[i * 4 + 2], floatData[i * 4 + 3]);
        }
}

void GLTFLoader::readAccessorUVec4(Model *gltf, Index accessorIndex, std::vector<glm::uvec4> &outData)
{
        const Accessor &accessor = gltf->accessors[accessorIndex];
        outData.resize(accessor.count);

        const uint8_t *data = getAccessorData(gltf, accessorIndex);

        // Joints can be stored as different types
        switch (accessor.componentType)
        {
        case Accessor::ComponentType::UByte:
        {
                const uint8_t *ubyteData = reinterpret_cast<const uint8_t *>(data);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                        outData[i] = glm::uvec4(ubyteData[i * 4],
                                                ubyteData[i * 4 + 1],
                                                ubyteData[i * 4 + 2],
                                                ubyteData[i * 4 + 3]);
                }
                break;
        }
        case Accessor::ComponentType::UShort:
        {
                const uint16_t *ushortData = reinterpret_cast<const uint16_t *>(data);
                for (size_t i = 0; i < accessor.count; ++i)
                {
                        outData[i] = glm::uvec4(ushortData[i * 4],
                                                ushortData[i * 4 + 1],
                                                ushortData[i * 4 + 2],
                                                ushortData[i * 4 + 3]);
                }
                break;
        }
        default:
                IC_CORE_ERROR("Unsupported component type for joint indices");
                break;
        }
}

void GLTFLoader::loadBufferView(Model *gltf, fastgltf::BufferView &bufferView)
{
        BufferView bufView;

        bufView.bufferIndex = bufferView.bufferIndex;
        bufView.byteOffset  = bufferView.byteOffset;
        bufView.byteLength  = bufferView.byteLength;
        bufView.byteStride  = bufferView.byteStride.value_or(0);

        bufView.name        = bufferView.name;

        gltf->bufferViews.push_back(std::move(bufView));
}

void GLTFLoader::loadBuffer(Model *gltf, const fastgltf::Buffer &buffer, const std::filesystem::path &basePath)
{
        Buffer buf;
        std::visit(fastgltf::visitor{[&](const fastgltf::sources::Array &array)
                                     {
                                             buf.data.resize(array.bytes.size());
                                             memcpy(buf.data.data(), array.bytes.data(), array.bytes.size());
                                     },
                                     [&](const fastgltf::sources::Vector &vector)
                                     {
                                             buf.data.resize(vector.bytes.size());
                                             memcpy(buf.data.data(), vector.bytes.data(), vector.bytes.size());
                                     },
                                     [&](const fastgltf::sources::ByteView &view)
                                     {
                                             buf.data.resize(view.bytes.size());
                                             memcpy(buf.data.data(), view.bytes.data(), view.bytes.size());
                                     },
                                     [&](const fastgltf::sources::URI &uri)
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
                                             file.read(reinterpret_cast<char *>(buf.data.data()), fileSize);
                                     },
                                     [&](auto &&arg) { IC_CORE_WARN("Unsupported buffer source type"); }},
                   buffer.data);
        gltf->buffers.push_back(std::move(buf));
}

void GLTFLoader::loadAccessor(Model *gltf, fastgltf::Accessor &accessor)
{
        Accessor acc;
        acc.bufferView      = accessor.bufferViewIndex.has_value() ? toIndex(accessor.bufferViewIndex.value())
                                                                   : INVALID_INDEX;
        acc.offset          = accessor.byteOffset;
        acc.count           = accessor.count;
        acc.type            = convertAccessorType(accessor.type);

        acc.componentType   = static_cast<Accessor::ComponentType>(accessor.componentType);
        acc.normalized      = accessor.normalized;

        uint32_t components = getAccessorComponentCount(acc.type);

        // Extract min/max values from AccessorBoundsArray
        if (accessor.min.has_value())
        {
                const auto &minArray = accessor.min.value();
                acc.min.resize(minArray.size());

                // Check which type the bounds array holds and extract accordingly
                if (minArray.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
                {
                        const double *data = minArray.data<double>();
                        for (size_t i = 0; i < minArray.size(); ++i)
                        {
                                acc.min[i] = data[i];
                        }
                }
                else if (minArray.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
                {
                        const int64_t *data = minArray.data<int64_t>();
                        for (size_t i = 0; i < minArray.size(); ++i)
                        {
                                acc.min[i] = static_cast<double>(data[i]);
                        }
                }
        }

        if (accessor.max.has_value())
        {
                const auto &maxArray = accessor.max.value();
                acc.max.resize(maxArray.size());

                if (maxArray.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
                {
                        const double *data = maxArray.data<double>();
                        for (size_t i = 0; i < maxArray.size(); ++i)
                        {
                                acc.max[i] = data[i];
                        }
                }
                else if (maxArray.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
                {
                        const int64_t *data = maxArray.data<int64_t>();
                        for (size_t i = 0; i < maxArray.size(); ++i)
                        {
                                acc.max[i] = static_cast<double>(data[i]);
                        }
                }
        }

        gltf->accessors.push_back(std::move(acc));
}

bool GLTFLoader::loadImage(Model *gltf, fastgltf::Asset &asset, fastgltf::Image &image)
{
        ImageData imageData;

        std::visit(
            fastgltf::visitor{
                [](auto &arg) {},
                [&](fastgltf::sources::URI &filePath)
                {
                        IC_CORE_ASSERT(filePath.fileByteOffset == 0, "STBI Offset error!");
                        IC_CORE_ASSERT(filePath.uri.isLocalPath(), "Did not find the URI path");
                        int width, height, nrChannels;

                        const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
                        uint8_t *data      = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                        imageData.channels = nrChannels;
                        imageData.width    = width;
                        imageData.height   = height;

                        size_t size        = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
                        imageData.pixels.assign(data, data + size);
                        stbi_image_free(data);
                },
                [&](fastgltf::sources::Array &vector)
                {
                        int width, height, nrChannels;
                        uint8_t *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(vector.bytes.data()),
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
                [&](fastgltf::sources::BufferView &view)
                {
                        auto &bufferView = asset.bufferViews[view.bufferViewIndex];
                        auto &buffer     = asset.buffers[bufferView.bufferIndex];
                        std::visit(fastgltf::visitor{[](auto &arg) {},
                                                     [&](fastgltf::sources::Array &vector)
                                                     {
                                                             int width, height, nrChannels;
                                                             unsigned char *data = stbi_load_from_memory(
                                                                 reinterpret_cast<const stbi_uc *>(
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

void GLTFLoader::loadTexture(Model *gltf, fastgltf::Texture &texture)
{
        Texture tex;

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

bool GLTFLoader::loadCamera(Model *gltf, fastgltf::Camera &camera)
{
        return false;
}

static GLenum toGLenum(fastgltf::ComponentType type)
{
        return static_cast<GLenum>(static_cast<uint16_t>(type) & 0x1FFF);
}

// Helper to get component count from accessor type
static size_t getAccessorComponentCount(Accessor::Type type)
{
        switch (type)
        {
        case Accessor::Type::SCALAR:
                return 1;
        case Accessor::Type::VEC2:
                return 2;
        case Accessor::Type::VEC3:
                return 3;
        case Accessor::Type::VEC4:
                return 4;
        case Accessor::Type::MAT4:
                return 16;
        default:
                return 0;
        }
}

// Helper to get component size from GL type
static size_t getComponentSize(GLenum componentType)
{
        switch (componentType)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
                return 1;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
                return 2;
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
                return 4;
        default:
                return 0;
        }
}

// Helper to extract values from AccessorBoundsArray
static std::vector<double> extractBoundsArray(const fastgltf::AccessorBoundsArray &bounds)
{
        std::vector<double> result(bounds.size());

        if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
        {
                const double *data = bounds.data<double>();
                std::copy(data, data + bounds.size(), result.begin());
        }
        else if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
        {
                const int64_t *data = bounds.data<int64_t>();
                std::transform(data,
                               data + bounds.size(),
                               result.begin(),
                               [](int64_t val) { return static_cast<double>(val); });
        }

        return result;
}

// Helper to get bounding box from accessor min/max
static glm::vec3 getBoundingBoxMin(const Accessor &accessor)
{
        if (accessor.min.size() >= 3)
        {
                return glm::vec3(static_cast<float>(accessor.min[0]),
                                 static_cast<float>(accessor.min[1]),
                                 static_cast<float>(accessor.min[2]));
        }
        return glm::vec3(0.0f);
}

static glm::vec3 getBoundingBoxMax(const Accessor &accessor)
{
        if (accessor.max.size() >= 3)
        {
                return glm::vec3(static_cast<float>(accessor.max[0]),
                                 static_cast<float>(accessor.max[1]),
                                 static_cast<float>(accessor.max[2]));
        }
        return glm::vec3(0.0f);
}

}  // namespace ic
