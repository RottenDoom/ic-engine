#include "core/assets/types/model.h"
#include "core/assets/asset_loaders/gltf_loader.h"
#include "core/assets/asset_loaders/model_data.h"

/**
 * model_builder.cpp
 *
 * ic::buildModel() -> the single translation point from ModelImportData → Model.
 *
 * This is the ONLY place in the codebase that writes into Model's private
 * members. It is a free function in namespace ic, not a class, so it grants
 * minimal access: exactly one function, not an entire class scope.
 *
 * Responsibilities:
 *   1. Pack ic::Vertex arrays from import data into interleaved vertexData blobs
 *   2. Compute attributeFlags and vertexStride per primitive
 *   3. Build the node hierarchy (compute worldTransforms, fill parent indices)
 *   4. Merge AABBs from primitives → meshes → world
 *   5. Translate all ImportData types to their runtime equivalents
 *
 */

namespace ic
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Compute interleaved vertex stride and attribute flags from import data. */
static void computeVertexLayout(const MeshPrimitiveImportData *src, uint32_t &outFlags, uint32_t &outStride)
{
        outFlags  = ATTRIB_POSITION;  // position is always present
        outStride = sizeof(glm::vec3);

        // Detect presence by checking the import-time booleans we set during extraction.
        // Fall back to checking the actual vertex data if the flags aren't set.
        // We use the first vertex as the canonical reference.

        // if (src.primitives.empty())
        // {
        //         outFlags  = ATTRIB_NONE;
        //         outStride = 0;
        //         return;
        // }

        // We inspect the import data's primitives to decide -> but buildModel receives
        // MeshPrimitiveImportData directly, so we just check non-zero values in
        // vertices[0] as the loader already does. The correct approach is to use
        // the TempPrimitiveData flags, but those are gone by the time we get here.
        // Instead, we check each attribute slot on the first vertex.

        if (!src->vertices.empty())
        {
                const Vertex &v = src->vertices[0];

                if (glm::length(v.normal) > 0.0f)
                {
                        outFlags  |= ATTRIB_NORMAL;
                        outStride += sizeof(glm::vec3);
                }
                if (glm::length(glm::vec3(v.tangent)) > 0.0f)
                {
                        outFlags  |= ATTRIB_TANGENT;
                        outStride += sizeof(glm::vec4);
                }
                if (v.uv0 != glm::vec2(0.0f))
                {
                        outFlags  |= ATTRIB_TEXCOORD0;
                        outStride += sizeof(glm::vec2);
                }
                if (v.uv1 != glm::vec2(0.0f))
                {
                        outFlags  |= ATTRIB_TEXCOORD1;
                        outStride += sizeof(glm::vec2);
                }
                if (v.uv2 != glm::vec2(0.0f))
                {
                        outFlags  |= ATTRIB_TEXCOORD2;
                        outStride += sizeof(glm::vec2);
                }
                if (v.color != glm::vec4(0.0f))
                {
                        outFlags  |= ATTRIB_COLOR;
                        outStride += sizeof(glm::vec4);
                }
                if (v.joint0 != glm::uvec4(0))
                {
                        outFlags  |= ATTRIB_JOINTS;
                        outStride += sizeof(glm::uvec4);
                }
                if (glm::length(v.weight0) > 0.0f)
                {
                        outFlags  |= ATTRIB_WEIGHTS;
                        outStride += sizeof(glm::vec4);
                }
        }
}

/**
 * Pack an array of ic::Vertex structs into an interleaved byte buffer
 * based on the provided attributeFlags and stride.
 * Attribute order must match GLPrimitive::setupVertexAttributes().
 */
static std::vector<uint8_t> packVertices(const std::vector<Vertex> &vertices, uint32_t flags, uint32_t stride)
{
        const size_t         count = vertices.size();
        std::vector<uint8_t> buf(count * stride, 0);

        for (size_t i = 0; i < count; ++i)
        {
                const Vertex &v      = vertices[i];
                uint8_t      *dst    = buf.data() + i * stride;
                size_t        offset = 0;

                auto write = [&](const void *src, size_t sz)
                {
                        memcpy(dst + offset, src, sz);
                        offset += sz;
                };

                if (flags & ATTRIB_POSITION)
                        write(&v.pos, sizeof(glm::vec3));
                if (flags & ATTRIB_NORMAL)
                        write(&v.normal, sizeof(glm::vec3));
                if (flags & ATTRIB_TANGENT)
                        write(&v.tangent, sizeof(glm::vec4));
                if (flags & ATTRIB_TEXCOORD0)
                        write(&v.uv0, sizeof(glm::vec2));
                if (flags & ATTRIB_TEXCOORD1)
                        write(&v.uv1, sizeof(glm::vec2));
                if (flags & ATTRIB_TEXCOORD2)
                        write(&v.uv2, sizeof(glm::vec2));
                if (flags & ATTRIB_COLOR)
                        write(&v.color, sizeof(glm::vec4));
                if (flags & ATTRIB_JOINTS)
                        write(&v.joint0, sizeof(glm::uvec4));
                if (flags & ATTRIB_WEIGHTS)
                        write(&v.weight0, sizeof(glm::vec4));
        }

        return buf;
}

// ---------------------------------------------------------------------------
// Node hierarchy pass
// Fills parent indices and computes worldTransform for every node.
// ---------------------------------------------------------------------------

static void buildHierarchy(std::vector<Node> &nodes, Index nodeIdx, const glm::mat4 &parentWorld)
{
        if (nodeIdx >= nodes.size())
                return;
        Node &node          = nodes[nodeIdx];
        node.worldTransform = parentWorld * node.localTransform;

        for (Index childIdx : node.children)
        {
                if (childIdx < nodes.size())
                {
                        nodes[childIdx].parent = nodeIdx;
                        buildHierarchy(nodes, childIdx, node.worldTransform);
                }
        }
}

// ---------------------------------------------------------------------------
// Primitive builder
// ---------------------------------------------------------------------------

static MeshPrimitive buildPrimitive(MeshPrimitiveImportData *src)
{
        MeshPrimitive prim;
        prim.mode          = static_cast<MeshPrimitive::Mode>(src->mode);
        prim.materialIndex = src->materialIndex;
        prim.morphWeights  = std::move(src->morphWeights);
        prim.bounds.min    = src->aabbMin;
        prim.bounds.max    = src->aabbMax;

        uint32_t flags  = ATTRIB_NONE;
        uint32_t stride = 0;
        computeVertexLayout(src, flags, stride);

        prim.attributeFlags = flags;
        prim.vertexStride   = stride;
        prim.vertexCount    = static_cast<uint32_t>(src->vertices.size());
        prim.vertexData     = packVertices(src->vertices, flags, stride);
        prim.indices        = std::move(src->indices);

        return prim;
}

// ---------------------------------------------------------------------------
// Material translation
// Converts MaterialImportData → runtime Material.
// TextureRef resolution: import data stores texture-list index in idx.
// The texture list maps texture index → (image index, sampler index).
// We pre-resolve image+sampler here so the renderer never touches textures[].
// ---------------------------------------------------------------------------

static TextureRef resolveTextureRef(const TextureRefImportData &ref, const std::vector<TextureImportData> &textures)
{
        TextureRef out;
        if (ref.idx == INVALID_INDEX)
                return out;
        if (ref.idx >= textures.size())
                return out;

        const TextureImportData &tex = textures[ref.idx];
        out.image                    = tex.image;
        out.sampler                  = tex.sampler;
        out.texCoord                 = ref.texCoord;
        return out;
}

static Material buildMaterial(const MaterialImportData &src, const std::vector<TextureImportData> &textures)
{
        Material mat;
        mat.name = src.name;

        // PBR
        mat.pbr.baseColorFactor          = src.pbr.baseColorFactor;
        mat.pbr.metallicFactor           = src.pbr.metallicFactor;
        mat.pbr.roughnessFactor          = src.pbr.roughnessFactor;
        mat.pbr.baseColorTexture         = resolveTextureRef(src.pbr.baseColorTexture, textures);
        mat.pbr.metallicRoughnessTexture = resolveTextureRef(src.pbr.metallicRoughnessTexture, textures);

        // Normal
        mat.normalTexture.ref   = resolveTextureRef(src.normalTexture.ref, textures);
        mat.normalTexture.scale = src.normalTexture.scale;

        // Occlusion
        mat.occlusionTexture.ref      = resolveTextureRef(src.occlusionTexture.ref, textures);
        mat.occlusionTexture.strength = src.occlusionTexture.strength;

        // Emissive
        mat.emissiveTexture  = resolveTextureRef(src.emissiveTexture, textures);
        mat.emissiveFactor   = src.emissiveFactor;
        mat.emissiveStrength = 1.0f;  // KHR_materials_emissive_strength not yet in import data

        // Alpha
        mat.alphaMode   = static_cast<Material::AlphaMode>(src.alphaMode);
        mat.alphaCutoff = src.alphaCutoff;

        // Surface
        mat.doubleSided = src.doubleSided;

        return mat;
}

// ---------------------------------------------------------------------------
// ic::buildModel -> the single entry point
// ---------------------------------------------------------------------------

Model buildModel(ModelImportData *data, GUID id)
{
        Model model(id);

        // --- Images ---
        model.m_images.reserve(data->images.size());
        for (auto &img : data->images)
        {
                Image out;
                out.width    = img.width;
                out.height   = img.height;
                out.channels = img.channels;
                out.srgb     = img.srgb;
                out.pixels   = std::move(img.pixels);
#if defined(IC_ASSET_NAMES)
                out.name = img.name;
#endif
                model.m_images.push_back(std::move(out));
        }

        // --- Samplers ---
        model.m_samplers.reserve(data->samplers.size());
        for (auto &s : data->samplers)
        {
                Sampler out;
                out.magFilter = static_cast<Sampler::Filter>(s.magFilter);
                out.minFilter = static_cast<Sampler::Filter>(s.minFilter);
                out.wrapS     = static_cast<Sampler::Wrap>(s.wrapS);
                out.wrapT     = static_cast<Sampler::Wrap>(s.wrapT);
                model.m_samplers.push_back(out);
        }

        // --- Materials (resolve texture refs against import texture list) ---
        model.m_materials.reserve(data->materials.size());
        for (const auto &mat : data->materials)
                model.m_materials.push_back(buildMaterial(mat, data->textures));

        // Note: data.textures is NOT stored on Model.
        // All texture references are now resolved to (image, sampler) index pairs.

        // --- Cameras ---
        model.m_cameras.reserve(data->cameras.size());
        for (const auto &cam : data->cameras)
        {
                ModelCamera out;
                out.name         = cam.name;
                out.type         = static_cast<ModelCamera::Type>(cam.type);
                out.perspective  = {cam.perspective.aspectRatio,
                                    cam.perspective.yfov,
                                    cam.perspective.zfar,
                                    cam.perspective.znear};
                out.orthographic = {cam.orthographic.xmag,
                                    cam.orthographic.ymag,
                                    cam.orthographic.zfar,
                                    cam.orthographic.znear};
                model.m_cameras.push_back(std::move(out));
        }

        // --- Skins ---
        model.m_skins.reserve(data->skins.size());
        for (auto &skin : data->skins)
        {
                Skin out;
                out.name                = skin.name;
                out.jointIndices        = std::move(skin.jointIndices);
                out.inverseBindMatrices = std::move(skin.inverseBindMatrices);
                out.skeletonRootIndex   = skin.skeletonRootIndex;
                model.m_skins.push_back(std::move(out));
        }

        // --- Animations ---
        model.m_animations.reserve(data->animations.size());
        for (auto &anim : data->animations)
        {
                Animation out;
                out.name     = anim.name;
                out.duration = anim.duration;

                out.samplers.reserve(anim.samplers.size());
                for (auto &s : anim.samplers)
                {
                        AnimationSampler as;
                        as.interpolation = static_cast<AnimationSampler::Interpolation>(s.interpolation);
                        as.inputTimes    = std::move(s.inputTimes);
                        as.outputValues  = std::move(s.outputValues);
                        out.samplers.push_back(std::move(as));
                }

                out.channels.reserve(anim.channels.size());
                for (const auto &ch : anim.channels)
                {
                        AnimationChannel ac;
                        ac.targetPath      = static_cast<AnimationChannel::Path>(ch.targetPath);
                        ac.samplerIndex    = ch.samplerIndex;
                        ac.targetNodeIndex = ch.targetNodeIndex;
                        out.channels.push_back(ac);
                }

                model.m_animations.push_back(std::move(out));
        }

        // --- Nodes ---
        model.m_nodes.reserve(data->nodes.size());
        for (auto &n : data->nodes)
        {
                Node out;
                out.name           = n.name;
                out.translation    = n.translation;
                out.rotation       = n.rotation;
                out.scale          = n.scale;
                out.localTransform = n.localTransform;
                out.worldTransform = glm::mat4(1.0f);  // computed below
                out.meshIndex      = n.meshIndex;
                out.skinIndex      = n.skinIndex;
                out.cameraIndex    = n.cameraIndex;
                out.lightIndex     = n.lightIndex;
                out.children       = std::move(n.children);
                out.parent         = INVALID_INDEX;  // filled below
                model.m_nodes.push_back(std::move(out));
        }

        // --- Scenes ---
        model.m_scenes.reserve(data->scenes.size());
        for (auto &scene : data->scenes)
        {
                Scene out;
                out.name      = scene.name;
                out.rootNodes = std::move(scene.rootNodes);
                model.m_scenes.push_back(std::move(out));
        }

        model.m_defaultScene = data->defaultScene;

        // --- Node hierarchy pass ---
        // Walk from each scene's root nodes to compute worldTransforms
        // and fill parent indices depth-first.
        if (model.m_defaultScene < model.m_scenes.size())
        {
                const Scene &defaultScene = model.m_scenes[model.m_defaultScene];
                for (Index rootIdx : defaultScene.rootNodes)
                        buildHierarchy(model.m_nodes, rootIdx, glm::mat4(1.0f));
        }
        else
        {
                // No valid default scene -> walk all scenes
                for (const auto &scene : model.m_scenes)
                        for (Index rootIdx : scene.rootNodes)
                                buildHierarchy(model.m_nodes, rootIdx, glm::mat4(1.0f));
        }

        // --- Meshes (pack geometry, compute AABBs) ---
        model.m_meshes.reserve(data->meshes.size());
        for (auto &importMesh : data->meshes)
        {
                Mesh mesh;
                mesh.name       = importMesh.name;
                mesh.bounds.min = importMesh.aabbMin;
                mesh.bounds.max = importMesh.aabbMax;

                mesh.primitives.reserve(importMesh.primitives.size());
                for (auto &importPrim : importMesh.primitives)
                        mesh.primitives.push_back(buildPrimitive(&importPrim));  // see if this fixes things

                model.m_meshes.push_back(std::move(mesh));
        }

        // --- World AABB -> union of all mesh AABBs transformed by their node's world matrix ---
        for (const auto &node : model.m_nodes)
        {
                if (node.meshIndex == INVALID_INDEX)
                        continue;
                if (node.meshIndex >= model.m_meshes.size())
                        continue;

                const Mesh &mesh = model.m_meshes[node.meshIndex];
                if (!mesh.bounds.isValid())
                        continue;

                // Transform mesh AABB corners by worldTransform
                const glm::mat4 &W          = node.worldTransform;
                glm::vec3        corners[8] = {
                    glm::vec3(W * glm::vec4(mesh.bounds.min.x, mesh.bounds.min.y, mesh.bounds.min.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.max.x, mesh.bounds.min.y, mesh.bounds.min.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.min.x, mesh.bounds.max.y, mesh.bounds.min.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.max.x, mesh.bounds.max.y, mesh.bounds.min.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.min.x, mesh.bounds.min.y, mesh.bounds.max.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.max.x, mesh.bounds.min.y, mesh.bounds.max.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.min.x, mesh.bounds.max.y, mesh.bounds.max.z, 1.f)),
                    glm::vec3(W * glm::vec4(mesh.bounds.max.x, mesh.bounds.max.y, mesh.bounds.max.z, 1.f)),
                };
                for (const auto &c : corners)
                        model.m_worldBounds.expand(c);
        }

        // State is set by Model::load() after this function returns.
        // buildModel() produces a logically CPUReady model but does not
        // touch m_state -> that's the caller's responsibility.

        return model;
}

bool Model::load(const char *filepath)
{
        IC_CORE_ASSERT(filepath, "Model::load -> null filepath");

        // TODO: if already loaded increase the refcount of the asset
        if (m_state == State::CPUReady || m_state == State::GPUReady)
        {
                IC_CORE_WARN("Model::load -> already loaded, call unload() first");
                return false;
        }

        m_state = State::Pending;

        // -----------------------------------------------------------------------
        // Pick loader by extension
        // -----------------------------------------------------------------------
        const char *ext = fs_getExtension(filepath);  // returns "gltf", "glb", etc.

        // Currently only GLTF is supported. When OBJ/FBX loaders exist,
        // this becomes a registry lookup: LoaderRegistry::getFor(ext)
        GLTFLoader loader;
        if (!loader.canLoad(ext))
        {
                IC_CORE_ERROR("Model::load -> no loader registered for extension '{}'", ext ? ext : "(null)");
                m_state = State::Failed;
                return false;
        }

        // -----------------------------------------------------------------------
        // Load into import data (pure CPU, no shared state)
        // -----------------------------------------------------------------------
        ModelImportData importData;
        if (!loader.load(filepath, &importData))
        {
                IC_CORE_ERROR("Model::load -> loader failed for '{}'", filepath);
                m_state = State::Failed;
                return false;
        }

        // -----------------------------------------------------------------------
        // Build runtime model from import data
        // buildModel() is the only function that writes into Model's private data.
        // -----------------------------------------------------------------------
        *this   = buildModel(&importData, getID());
        m_state = State::CPUReady;

        IC_CORE_INFO("Model::load -> '{}' ready ({} meshes, {} materials, {} nodes)",
                     filepath,
                     m_meshes.size(),
                     m_materials.size(),
                     m_nodes.size());

        return true;
}

bool Model::release()
{
        // Free CPU geometry
        for (auto &mesh : m_meshes)
        {
                for (auto &prim : mesh.primitives)
                {
                        prim.vertexData.clear();
                        prim.vertexData.shrink_to_fit();
                        prim.indices.clear();
                        prim.indices.shrink_to_fit();
                }
        }

        // Free CPU image data
        for (auto &img : m_images)
        {
                img.pixels.clear();
                img.pixels.shrink_to_fit();
        }

        m_meshes.clear();
        m_materials.clear();
        m_images.clear();
        m_samplers.clear();
        m_nodes.clear();
        m_cameras.clear();
        m_skins.clear();
        m_animations.clear();
        m_scenes.clear();

        m_worldBounds  = AABB::makeInvalid();
        m_defaultScene = INVALID_INDEX;
        m_state        = State::Unloaded;

        return true;
}

void Model::freeCPU()
{
        // Called after successful GPU upload.
        // Releases vertex data and pixel data -> keeps everything else
        // (materials, nodes, cameras, animations) since the CPU needs those.

        for (auto &mesh : m_meshes)
        {
                for (auto &prim : mesh.primitives)
                {
                        prim.vertexData.clear();
                        prim.vertexData.shrink_to_fit();
                        // Keep indices -> needed for CPU-side ray intersection queries
                        // If you never do CPU raycasting, clear these too.
                }
        }

        for (auto &img : m_images)
        {
                img.pixels.clear();
                img.pixels.shrink_to_fit();
        }

        // State stays GPUReady -> the data is on the GPU, not gone.
}

bool Model::serializedLoad(ic::Serializer * /*s*/)
{
        // TODO: implement fast .icache binary deserialization
        // Sequence:
        //   1. Read + validate header (magic number, version, asset type)
        //   2. Deserialize meshes: for each primitive, read vertexCount,
        //      vertexStride, attributeFlags, then memcpy vertexData and indices
        //   3. Deserialize images: read width/height/channels/srgb, then pixels blob
        //   4. Deserialize materials, samplers, nodes, cameras, skins, animations, scenes
        //   5. Read worldBounds and defaultScene
        //   6. Set m_state = State::CPUReady
        IC_CORE_WARN("Model::serializedLoad -> not yet implemented");
        return false;
}

bool Model::serializedSave(ic::Serializer * /*s*/) const
{
        // TODO: implement fast .icache binary serialization
        // Mirror of serializedLoad. Write only CPUReady data -> no GLTF intermediates.
        IC_CORE_WARN("Model::serializedSave -> not yet implemented");
        return false;
}

}  // namespace ic