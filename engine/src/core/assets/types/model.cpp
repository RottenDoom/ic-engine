#include "core/assets/types/model.h"
#include "core/assets/asset_serializer.h"
#include "core/assets/asset_manager.h"
#include "core/assets/asset_versions.h"
#include "core/assets/asset_cache.h"
#include "core/assets/asset_loaders/gltf_loader.h"
#include "core/assets/asset_loaders/model_data.h"
#include "core/assets/types/texture.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * model_builder.cpp
 *
 * ic::build_model() -> the single translation point from ModelImportData → Model.
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
static void compute_vertex_layout(const MeshPrimitiveImportData *src, uint32_t &outFlags, uint32_t &outStride)
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

        // We inspect the import data's primitives to decide -> but build_model receives
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
static std::vector<uint8_t> pack_vertices(const std::vector<Vertex> &vertices, uint32_t flags, uint32_t stride)
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

static void build_hierarchy(std::vector<Node> &nodes, Index nodeIdx, const glm::mat4 &parentWorld)
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
                        build_hierarchy(nodes, childIdx, node.worldTransform);
                }
        }
}

// ---------------------------------------------------------------------------
// Primitive builder
// ---------------------------------------------------------------------------

/**
 * Creates (or reuses) a Texture asset for the given import ref and returns its handle.
 * Image pixels are moved out of modelData on first use, so dedup is required.
 */
static TextureHandle resolve_texture_ref(const TextureRefImportData                  &ref,
                                         ModelImportData                             *modelData,
                                         std::unordered_map<uint32_t, TextureHandle> &textureCache)
{
        if (ref.imageIdx == INVALID_INDEX || ref.imageIdx >= modelData->images.size())
                return INVALID_ID;

        // Dedup on image index: one Texture asset per source image.
        // TODO: key on (image, sampler) once a texture owns its sampler identity.
        auto it = textureCache.find(ref.imageIdx);
        if (it != textureCache.end())
                return it->second;

        TextureHandle id  = UUIDGenerator::Generate();
        Texture      *tex = AssetManager::Get().CreateAsset<Texture>(id, false);
        if (tex)
        {
                tex->LoadFromImageData(modelData->images[ref.imageIdx]);
                if (ref.samplerIdx != INVALID_INDEX && ref.samplerIdx < modelData->samplers.size())
                        tex->SetSampler(modelData->samplers[ref.samplerIdx]);
        }

        textureCache[ref.imageIdx] = id;
        return id;
}

/** Translates an import material into a runtime Material, resolving texture refs into Texture assets. */
static Material build_material(const MaterialImportData                    &src,
                               ModelImportData                             *modelData,
                               std::unordered_map<uint32_t, TextureHandle> &textureCache)
{
        Material mat;
        mat.name = src.name;

        // PBR
        mat.pbr.baseColorFactor          = src.pbr.baseColorFactor;
        mat.pbr.metallicFactor           = src.pbr.metallicFactor;
        mat.pbr.roughnessFactor          = src.pbr.roughnessFactor;
        mat.pbr.baseColorTexture         = resolve_texture_ref(src.pbr.baseColorTexture, modelData, textureCache);
        mat.pbr.metallicRoughnessTexture = resolve_texture_ref(src.pbr.metallicRoughnessTexture,
                                                               modelData,
                                                               textureCache);

        // Normal
        mat.normalTexture.ref   = resolve_texture_ref(src.normalTexture.ref, modelData, textureCache);
        mat.normalTexture.scale = src.normalTexture.scale;

        // Occlusion
        mat.occlusionTexture.ref      = resolve_texture_ref(src.occlusionTexture.ref, modelData, textureCache);
        mat.occlusionTexture.strength = src.occlusionTexture.strength;

        // Emissive
        mat.emissiveTexture.emissiveTexture  = resolve_texture_ref(src.emissiveTexture, modelData, textureCache);
        mat.emissiveTexture.emissiveFactor   = src.emissiveFactor;
        mat.emissiveTexture.emissiveStrength = 1.0f;  // KHR_materials_emissive_strength not yet in import data

        // Alpha
        mat.alphaMode   = static_cast<Material::AlphaMode>(src.alphaMode);
        mat.alphaCutoff = src.alphaCutoff;

        // Surface
        mat.doubleSided = src.doubleSided;

        return mat;
}

/** Packs geometry from import data into a runtime primitive and assigns its material handle. */
static MeshPrimitive build_primitive(MeshPrimitiveImportData *src, MaterialHandle materialHandle)
{
        MeshPrimitive prim;

        prim.mode = static_cast<MeshPrimitive::Mode>(src->mode);

        // setup primitive bouunds and weights
        prim.morphWeights = std::move(src->morphWeights);
        prim.bounds.min   = src->aabbMin;
        prim.bounds.max   = src->aabbMax;

        uint32_t flags  = ATTRIB_NONE;
        uint32_t stride = 0;
        compute_vertex_layout(src, flags, stride);

        // setup vertex attribs and pack vertices
        prim.attributeFlags = flags;
        prim.vertexStride   = stride;
        prim.vertexCount    = static_cast<uint32_t>(src->vertices.size());
        prim.vertexData     = pack_vertices(src->vertices, flags, stride);
        prim.indices        = std::move(src->indices);

        prim.materialHandle = materialHandle;

        return prim;
}

// ---------------------------------------------------------------------------
// ic::build_model -> the single entry point
// ---------------------------------------------------------------------------

Model build_model(ModelImportData *data, IC_GUID id)
{
        Model model(id);

        // create material with already build texture cache in asset manager
        std::unordered_map<uint32_t, TextureHandle> textureCache;
        std::vector<MaterialHandle>                 materialHandles(data->materials.size());
        {
                AssetManager &mgr = AssetManager::Get();
                for (size_t i = 0; i < data->materials.size(); ++i)
                {
                        Material       mat   = build_material(data->materials[i], data, textureCache);
                        MaterialHandle matId = UUIDGenerator::Generate();
                        MaterialAsset *asset = mgr.CreateAsset<MaterialAsset>(matId, false);
                        if (asset)
                        {
                                asset->SetName(mat.name.c_str());
                                asset->SetMaterial(mat);
                        }
                        materialHandles[i] = matId;
                }
        }

        // Pack vertices and compute AABBs
        model.m_meshes.reserve(data->meshes.size());
        for (auto &importMesh : data->meshes)
        {
                Mesh mesh;
                mesh.name       = importMesh.name.c_str();
                mesh.bounds.min = importMesh.aabbMin;
                mesh.bounds.max = importMesh.aabbMax;

                mesh.primitives.reserve(importMesh.primitives.size());
                for (auto &importPrim : importMesh.primitives)
                {
                        MaterialHandle mh = (importPrim.materialIndex != INVALID_INDEX &&
                                             importPrim.materialIndex < materialHandles.size())
                                                ? materialHandles[importPrim.materialIndex]
                                                : INVALID_ID;
                        mesh.primitives.push_back(build_primitive(&importPrim, mh));
                }

                model.m_meshes.push_back(std::move(mesh));
        }

        // Cameras
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

        // Skins
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

        // Animations
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

        // Nodes
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

        // Scenes
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
                        build_hierarchy(model.m_nodes, rootIdx, glm::mat4(1.0f));
        }
        else
        {
                // No valid default scene -> walk all scenes
                for (const auto &scene : model.m_scenes)
                        for (Index rootIdx : scene.rootNodes)
                                build_hierarchy(model.m_nodes, rootIdx, glm::mat4(1.0f));
        }

        // World AABB -> union of all mesh AABBs transformed by their node's world matrix
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
                        model.m_worldBounds.Expand(c);
        }

        // State is set by Model::load() after this function returns.
        // build_model() produces a logically CPUReady model but does not
        // touch m_state -> that's the caller's responsibility.

        return model;
}

/** Loads the model using build_model function */
bool Model::Load(const char *filepath)
{
        IC_CORE_ASSERT(filepath, "Model::load -> null filepath");

        if (m_state == State::CPUReady || m_state == State::GPUReady)
        {
                IC_CORE_WARN("Model::load -> already loaded, call unload() first");
                return false;
        }

        m_state = State::Pending;

        const char *ext = fs_getExtension(filepath);

        // -----------------------------------------------------------------------
        // Try cache first
        // -----------------------------------------------------------------------
        const char *cachePath = AssetManager::Get().GetRegistry()->GetCachePath(GetID());
        if (cachePath && fs_exists(cachePath))
        {
                uint64_t srcTime   = fs_getLastModificationTime(filepath);
                uint64_t cacheTime = AssetCache::GetAssetTimeStamp(AssetType::ASSET_TYPE_MODEL, GetID());

                if (cacheTime >= srcTime)
                {
                        IC_CORE_INFO("Model::load -> Loading cached model from memory '{}'", filepath);
                        ic::Serializer s;
#ifndef NDEBUG
                        auto start = std::chrono::high_resolution_clock::now();
#endif
                        if (s.openForRead(cachePath) && SerializedLoad(&s))
                        {
                                s.close();
                                IC_CORE_INFO("Model::load -> loaded from cache '{}'", cachePath);
#ifndef NDEBUG
                                auto end = std::chrono::high_resolution_clock::now();
                                auto us  = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                                IC_CORE_INFO("Model::serializeLoad -> parsed '{}' in {:.2f} ms",
                                             cachePath,
                                             us / 1000.0);
#endif
                                ic_free(cachePath);
                                return true;
                        }
                        s.close();
                        IC_CORE_WARN("Model::load -> cache read failed, falling back to source");
                }
        }
        ic_free(cachePath);

        // cannot load from cache load from source file
        GLTFLoader loader;
        if (!loader.canLoad(ext))
        {
                IC_CORE_ERROR("Model::load -> no loader for extension '{}'", ext ? ext : "(null)");
                m_state = State::Failed;
                return false;
        }

        ModelImportData importData;

#ifndef NDEBUG
        auto start = std::chrono::high_resolution_clock::now();
#endif

        bool ok = loader.load(filepath, &importData);
        if (!ok)
        {
                IC_CORE_ERROR("Model::load -> loader failed for '{}'", filepath);
                m_state = State::Failed;
                return false;
        }

#ifndef NDEBUG
        auto end = std::chrono::high_resolution_clock::now();
        auto us  = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        IC_CORE_INFO("Model::load -> parsed '{}' in {:.2f} ms", filepath, us / 1000.0);
#endif
        // build up the intermediate model and setup the state for gpu upload
        *this   = build_model(&importData, GetID());
        m_state = State::CPUReady;

        // -----------------------------------------------------------------------
        // Write cache for next time
        // -----------------------------------------------------------------------
        if (!AssetCache::CacheAsset(AssetType::ASSET_TYPE_MODEL, GetID(), this))
                IC_CORE_WARN("Model::load -> failed to write cache for '{}'", filepath);

        IC_CORE_INFO("Model::load -> '{}' ready ({} meshes, {} nodes)", filepath, m_meshes.size(), m_nodes.size());
        return true;
}

bool Model::Release()
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

        m_meshes.clear();
        m_nodes.clear();
        m_cameras.clear();
        m_skins.clear();
        m_animations.clear();
        m_scenes.clear();

        m_worldBounds  = AABB::MakeInvalid();
        m_defaultScene = INVALID_INDEX;
        m_state        = State::Unloaded;

        return true;
}

void Model::FreeCPU()
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

        // Texture pixels live in their own Texture assets now; freeing them is the
        // AssetManager's concern, not the model's.

        // State stays GPUReady -> the data is on the GPU, not gone.
}

bool Model::SerializedSave(ic::Serializer *s) const
{
        IC_CORE_ASSERT(s && s->isWriting(), "serializedSave: serializer not open for write");

#ifndef NDEBUG
        IC_CORE_TRACE("serializedSave BEGIN '{}'", s->getFilename().c_str());
#endif

        // -----------------------------------------------------------------------
        // Header
        // -----------------------------------------------------------------------
        const uint32_t VERSION = static_cast<uint32_t>(assetCurrentVersion(ASSET_TYPE_MODEL));

        s->writePOD(IC_ASSET_MAGIC);
        s->writePOD(VERSION);

#ifndef NDEBUG
        IC_CORE_TRACE("  [header]  magic=0x{:08X} version={} @ {}", IC_ASSET_MAGIC, VERSION, s->tell());
#endif

        // -----------------------------------------------------------------------
        // Gather referenced materials (and their textures) from the AssetManager.
        // Materials/textures are not owned by the Model -> they live as assets and
        // are referenced by handle from primitives. We embed them in the cache file
        // (glTF-style) keyed by UUID so a warm load can reconstruct the assets.
        // -----------------------------------------------------------------------
        AssetManager &mgr = AssetManager::Get();

        std::vector<MaterialHandle>  matHandles;
        std::unordered_set<uint64_t> matSeen;
        for (const auto &mesh : m_meshes)
                for (const auto &prim : mesh.primitives)
                        if (prim.materialHandle != INVALID_ID && matSeen.insert(prim.materialHandle).second)
                                matHandles.push_back(prim.materialHandle);

        std::vector<const Material *> mats;
        mats.reserve(matHandles.size());
        std::vector<TextureHandle>   texHandles;
        std::unordered_set<uint64_t> texSeen;
        auto                         addTex = [&](TextureHandle h)
        {
                if (h != INVALID_ID && texSeen.insert(h).second)
                        texHandles.push_back(h);
        };
        for (MaterialHandle h : matHandles)
        {
                MaterialAsset  *ma = mgr.GetAsset<MaterialAsset>(h);
                const Material *m  = ma ? &ma->GetMaterial() : nullptr;
                mats.push_back(m);
                if (!m)
                {
                        IC_CORE_WARN("serializedSave: material {} not resident, writing default",
                                     static_cast<uint64_t>(h));
                        continue;
                }
                addTex(m->pbr.baseColorTexture);
                addTex(m->pbr.metallicRoughnessTexture);
                addTex(m->normalTexture.ref);
                addTex(m->occlusionTexture.ref);
                addTex(m->emissiveTexture.emissiveTexture);
        }

        // -----------------------------------------------------------------------
        // Textures (each = uuid + image pixels + sampler)
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [textures] start @ {}", s->tell());
#endif
                s->writePOD(static_cast<uint32_t>(texHandles.size()));

                for (TextureHandle h : texHandles)
                {
                        Texture       *tex = mgr.GetAsset<Texture>(h);
                        Image          empty;
                        Sampler        emptySampler;
                        const Image   &img  = tex ? *tex->GetImageTexture() : empty;
                        const Sampler &samp = tex ? tex->GetImageSampler() : emptySampler;

                        s->writePOD(h);
                        s->writePOD(img.width);
                        s->writePOD(img.height);
                        s->writePOD(img.channels);
                        s->writePOD(img.srgb);
                        s->writePOD(img.fromGLTF);
                        s->writeString(img.name.c_str());
                        s->writeString(img.uri.c_str());

                        uint64_t pixelBytes = static_cast<uint64_t>(img.pixels.size());
                        s->writePOD(pixelBytes);
                        if (pixelBytes > 0)
                                s->write(img.pixels.data(), static_cast<size_t>(pixelBytes));

                        s->writePOD(samp);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [textures] end   @ {} ({} textures)", s->tell(), texHandles.size());
#endif
        }

        // -----------------------------------------------------------------------
        // Materials (each = uuid + material data; texture refs are UUID handles)
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] start @ {}", s->tell());
#endif
                s->writePOD(static_cast<uint32_t>(matHandles.size()));

                for (size_t i = 0; i < matHandles.size(); ++i)
                {
                        Material        def;
                        const Material &mat = mats[i] ? *mats[i] : def;

                        s->writePOD(matHandles[i]);
                        s->writeString(mat.name.c_str());
                        s->writePOD(mat.pbr);               // PBRMetallicRoughness
                        s->writePOD(mat.normalTexture);     // NormalTexture
                        s->writePOD(mat.occlusionTexture);  // OcclusionTexture
                        s->writePOD(mat.emissiveTexture);   // EmissiveTexture
                        s->writePOD(mat.alphaMode);
                        s->writePOD(mat.alphaCutoff);
                        s->writePOD(mat.doubleSided);
                        s->writePOD(mat.unlit);
                        s->writePOD(mat.ior);
                        s->writePOD(mat.dispersion);

                        auto writeOpt = [&](const auto &opt)
                        {
                                bool has = opt.has_value();
                                s->writePOD(has);
                                if (has)
                                        s->writePOD(*opt);
                        };
                        writeOpt(mat.anisotropy);
                        writeOpt(mat.specular);
                        writeOpt(mat.iridescence);
                        writeOpt(mat.diffuseTransmission);
                        writeOpt(mat.transmission);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] end   @ {} ({} materials)", s->tell(), matHandles.size());
#endif
        }

        // -----------------------------------------------------------------------
        // Cameras
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [cameras] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_cameras.size());
                s->writePOD(count);

                for (const auto &cam : m_cameras)
                {
                        s->writeString(cam.name.c_str());
                        s->writePOD(cam.type);
                        s->writePOD(cam.perspective);
                        s->writePOD(cam.orthographic);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [cameras] end   @ {} ({} cameras)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Skins
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [skins] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_skins.size());
                s->writePOD(count);

                for (const auto &skin : m_skins)
                {
                        s->writeString(skin.name.c_str());
                        s->writePOD(skin.skeletonRootIndex);

                        uint32_t jointCount = static_cast<uint32_t>(skin.jointIndices.size());
                        s->writePOD(jointCount);
                        if (jointCount > 0)
                                s->write(skin.jointIndices.data(), jointCount * sizeof(Index));

                        uint32_t ibmCount = static_cast<uint32_t>(skin.inverseBindMatrices.size());
                        s->writePOD(ibmCount);
                        if (ibmCount > 0)
                                s->write(skin.inverseBindMatrices.data(), ibmCount * sizeof(glm::mat4));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [skins] end   @ {} ({} skins)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Animations
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [animations] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_animations.size());
                s->writePOD(count);

                for (const auto &anim : m_animations)
                {
                        s->writeString(anim.name.c_str());
                        s->writePOD(anim.duration);

                        uint32_t samplerCount = static_cast<uint32_t>(anim.samplers.size());
                        s->writePOD(samplerCount);
                        for (const auto &samp : anim.samplers)
                        {
                                s->writePOD(samp.interpolation);

                                uint32_t timeCount = static_cast<uint32_t>(samp.inputTimes.size());
                                s->writePOD(timeCount);
                                if (timeCount > 0)
                                        s->write(samp.inputTimes.data(), timeCount * sizeof(float));

                                uint32_t valCount = static_cast<uint32_t>(samp.outputValues.size());
                                s->writePOD(valCount);
                                if (valCount > 0)
                                        s->write(samp.outputValues.data(), valCount * sizeof(glm::vec4));
                        }

                        uint32_t chanCount = static_cast<uint32_t>(anim.channels.size());
                        s->writePOD(chanCount);
                        for (const auto &ch : anim.channels)
                        {
                                s->writePOD(ch.targetPath);
                                s->writePOD(ch.samplerIndex);
                                s->writePOD(ch.targetNodeIndex);
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [animations] end   @ {} ({} animations)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Nodes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [nodes] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_nodes.size());
                s->writePOD(count);

                for (const auto &node : m_nodes)
                {
                        s->writeString(node.name.c_str());
                        s->writePOD(node.translation);
                        s->writePOD(node.rotation);
                        s->writePOD(node.scale);
                        s->writePOD(node.localTransform);
                        s->writePOD(node.worldTransform);
                        s->writePOD(node.meshIndex);
                        s->writePOD(node.skinIndex);
                        s->writePOD(node.cameraIndex);
                        s->writePOD(node.lightIndex);
                        s->writePOD(node.parent);

                        uint32_t childCount = static_cast<uint32_t>(node.children.size());
                        s->writePOD(childCount);
                        if (childCount > 0)
                                s->write(node.children.data(), childCount * sizeof(Index));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [nodes] end   @ {} ({} nodes)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Scenes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [scenes] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_scenes.size());
                s->writePOD(count);

                for (const auto &scene : m_scenes)
                {
                        s->writeString(scene.name.c_str());
                        uint32_t rootCount = static_cast<uint32_t>(scene.rootNodes.size());
                        s->writePOD(rootCount);
                        if (rootCount > 0)
                                s->write(scene.rootNodes.data(), rootCount * sizeof(Index));
                }

                s->writePOD(m_defaultScene);
#ifndef NDEBUG
                IC_CORE_TRACE("  [scenes] end   @ {} ({} scenes, defaultScene={})", s->tell(), count, m_defaultScene);
#endif
        }

        // -----------------------------------------------------------------------
        // Meshes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [meshes] start @ {}", s->tell());
#endif
                uint32_t meshCount = static_cast<uint32_t>(m_meshes.size());
                s->writePOD(meshCount);

                for (const auto &mesh : m_meshes)
                {
                        s->writeString(mesh.name.c_str());
                        s->writePOD(mesh.bounds.min);
                        s->writePOD(mesh.bounds.max);

                        uint32_t primCount = static_cast<uint32_t>(mesh.primitives.size());
                        s->writePOD(primCount);

                        for (const auto &prim : mesh.primitives)
                        {
                                s->writePOD(prim.mode);
                                s->writePOD(prim.materialHandle);
                                s->writePOD(prim.attributeFlags);
                                s->writePOD(prim.vertexStride);
                                s->writePOD(prim.vertexCount);
                                s->writePOD(prim.bounds.min);
                                s->writePOD(prim.bounds.max);

                                uint32_t morphCount = static_cast<uint32_t>(prim.morphWeights.size());
                                s->writePOD(morphCount);
                                if (morphCount > 0)
                                        s->write(prim.morphWeights.data(), morphCount * sizeof(float));

                                uint64_t vbBytes = static_cast<uint64_t>(prim.vertexData.size());
                                s->writePOD(vbBytes);
                                if (vbBytes > 0)
                                        s->write(prim.vertexData.data(), static_cast<size_t>(vbBytes));

                                uint32_t idxCount = static_cast<uint32_t>(prim.indices.size());
                                s->writePOD(idxCount);
                                if (idxCount > 0)
                                        s->write(prim.indices.data(), idxCount * sizeof(uint32_t));
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [meshes] end   @ {} ({} meshes)", s->tell(), meshCount);
#endif
        }

        // -----------------------------------------------------------------------
        // World bounds
        // -----------------------------------------------------------------------
        s->writePOD(m_worldBounds.min);
        s->writePOD(m_worldBounds.max);

#ifndef NDEBUG
        IC_CORE_TRACE("serializedSave END @ {} bytes", s->tell());
#endif

        return true;
}

// =============================================================================

bool Model::SerializedLoad(ic::Serializer *s)
{
        IC_CORE_ASSERT(s && s->isReading(), "serializedLoad: serializer not open for read");

#ifndef NDEBUG
        IC_CORE_TRACE("serializedLoad BEGIN '{}'", s->getFilename().c_str());
#endif

        // -----------------------------------------------------------------------
        // Header
        // -----------------------------------------------------------------------
        constexpr uint32_t EXPECTED_MAGIC   = 0x49434D44;
        const uint32_t     EXPECTED_VERSION = static_cast<uint32_t>(assetCurrentVersion(ASSET_TYPE_MODEL));

        uint32_t magic = 0, version = 0;
        s->readPOD(magic);
        s->readPOD(version);

        if (magic != EXPECTED_MAGIC)
        {
                IC_CORE_ERROR("serializedLoad: bad magic 0x{:08X} (expected 0x{:08X}) in '{}'",
                              magic,
                              EXPECTED_MAGIC,
                              s->getFilename().c_str());
                return false;
        }
        if (version != EXPECTED_VERSION)
        {
                IC_CORE_WARN("serializedLoad: version mismatch -> cache={} current={} in '{}', needs rebuild",
                             version,
                             EXPECTED_VERSION,
                             s->getFilename().c_str());
                return false;
        }

#ifndef NDEBUG
        IC_CORE_TRACE("  [header]  magic=0x{:08X} version={} @ {}", magic, version, s->tell());
#endif

        AssetManager &mgr = AssetManager::Get();

        // -----------------------------------------------------------------------
        // Textures
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [textures] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);

                for (uint32_t i = 0; i < count; ++i)
                {
                        TextureHandle id;
                        s->readPOD(id);

                        Image img;
                        s->readPOD(img.width);
                        s->readPOD(img.height);
                        s->readPOD(img.channels);
                        s->readPOD(img.srgb);
                        s->readPOD(img.fromGLTF);
                        s->readString(img.name);
                        s->readString(img.uri);

                        uint64_t pixelBytes = 0;
                        s->readPOD(pixelBytes);
                        img.pixels.resize(static_cast<size_t>(pixelBytes));
                        if (pixelBytes > 0)
                                s->read(img.pixels.data(), static_cast<size_t>(pixelBytes));

                        Sampler samp;
                        s->readPOD(samp);

                        Texture *tex = mgr.CreateAsset<Texture>(id, false);
                        if (tex)
                        {
                                tex->SetImage(std::move(img));
                                tex->SetSampler(samp);
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [textures] end   @ {} ({} textures)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Materials -> reconstructed as MaterialAsset assets in the AssetManager
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);

                for (uint32_t i = 0; i < count; ++i)
                {
                        MaterialHandle id;
                        s->readPOD(id);

                        Material mat;
                        s->readString(mat.name);
                        s->readPOD(mat.pbr);
                        s->readPOD(mat.normalTexture);
                        s->readPOD(mat.occlusionTexture);
                        s->readPOD(mat.emissiveTexture);
                        s->readPOD(mat.alphaMode);
                        s->readPOD(mat.alphaCutoff);
                        s->readPOD(mat.doubleSided);
                        s->readPOD(mat.unlit);
                        s->readPOD(mat.ior);
                        s->readPOD(mat.dispersion);

                        auto readOpt = [&](auto &opt)
                        {
                                bool has = false;
                                s->readPOD(has);
                                if (has)
                                {
                                        typename std::decay_t<decltype(opt)>::value_type val;
                                        s->readPOD(val);
                                        opt = val;
                                }
                        };
                        readOpt(mat.anisotropy);
                        readOpt(mat.specular);
                        readOpt(mat.iridescence);
                        readOpt(mat.diffuseTransmission);
                        readOpt(mat.transmission);

                        MaterialAsset *ma = mgr.CreateAsset<MaterialAsset>(id, false);
                        if (ma)
                        {
                                ma->SetName(mat.name.c_str());
                                ma->SetMaterial(mat);
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] end   @ {} ({} materials)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Cameras
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [cameras] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_cameras.resize(count);

                for (auto &cam : m_cameras)
                {
                        s->readString(cam.name);
                        s->readPOD(cam.type);
                        s->readPOD(cam.perspective);
                        s->readPOD(cam.orthographic);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [cameras] end   @ {} ({} cameras)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Skins
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [skins] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_skins.resize(count);

                for (auto &skin : m_skins)
                {
                        s->readString(skin.name);
                        s->readPOD(skin.skeletonRootIndex);

                        uint32_t jointCount = 0;
                        s->readPOD(jointCount);
                        skin.jointIndices.resize(jointCount);
                        if (jointCount > 0)
                                s->read(skin.jointIndices.data(), jointCount * sizeof(Index));

                        uint32_t ibmCount = 0;
                        s->readPOD(ibmCount);
                        skin.inverseBindMatrices.resize(ibmCount);
                        if (ibmCount > 0)
                                s->read(skin.inverseBindMatrices.data(), ibmCount * sizeof(glm::mat4));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [skins] end   @ {} ({} skins)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Animations
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [animations] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_animations.resize(count);

                for (auto &anim : m_animations)
                {
                        s->readString(anim.name);
                        s->readPOD(anim.duration);

                        uint32_t samplerCount = 0;
                        s->readPOD(samplerCount);
                        anim.samplers.resize(samplerCount);

                        for (auto &samp : anim.samplers)
                        {
                                s->readPOD(samp.interpolation);

                                uint32_t timeCount = 0;
                                s->readPOD(timeCount);
                                samp.inputTimes.resize(timeCount);
                                if (timeCount > 0)
                                        s->read(samp.inputTimes.data(), timeCount * sizeof(float));

                                uint32_t valCount = 0;
                                s->readPOD(valCount);
                                samp.outputValues.resize(valCount);
                                if (valCount > 0)
                                        s->read(samp.outputValues.data(), valCount * sizeof(glm::vec4));
                        }

                        uint32_t chanCount = 0;
                        s->readPOD(chanCount);
                        anim.channels.resize(chanCount);

                        for (auto &ch : anim.channels)
                        {
                                s->readPOD(ch.targetPath);
                                s->readPOD(ch.samplerIndex);
                                s->readPOD(ch.targetNodeIndex);
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [animations] end   @ {} ({} animations)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Nodes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [nodes] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_nodes.resize(count);

                for (auto &node : m_nodes)
                {
                        s->readString(node.name);
                        s->readPOD(node.translation);
                        s->readPOD(node.rotation);
                        s->readPOD(node.scale);
                        s->readPOD(node.localTransform);
                        s->readPOD(node.worldTransform);
                        s->readPOD(node.meshIndex);
                        s->readPOD(node.skinIndex);
                        s->readPOD(node.cameraIndex);
                        s->readPOD(node.lightIndex);
                        s->readPOD(node.parent);

                        uint32_t childCount = 0;
                        s->readPOD(childCount);
                        node.children.resize(childCount);
                        if (childCount > 0)
                                s->read(node.children.data(), childCount * sizeof(Index));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [nodes] end   @ {} ({} nodes)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Scenes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [scenes] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_scenes.resize(count);

                for (auto &scene : m_scenes)
                {
                        s->readString(scene.name);
                        uint32_t rootCount = 0;
                        s->readPOD(rootCount);
                        scene.rootNodes.resize(rootCount);
                        if (rootCount > 0)
                                s->read(scene.rootNodes.data(), rootCount * sizeof(Index));
                }

                s->readPOD(m_defaultScene);
#ifndef NDEBUG
                IC_CORE_TRACE("  [scenes] end   @ {} ({} scenes, defaultScene={})", s->tell(), count, m_defaultScene);
#endif
        }

        // -----------------------------------------------------------------------
        // Meshes
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [meshes] start @ {}", s->tell());
#endif
                uint32_t meshCount = 0;
                s->readPOD(meshCount);
                m_meshes.resize(meshCount);

                for (auto &mesh : m_meshes)
                {
                        s->readString(mesh.name);
                        s->readPOD(mesh.bounds.min);
                        s->readPOD(mesh.bounds.max);

                        uint32_t primCount = 0;
                        s->readPOD(primCount);
                        mesh.primitives.resize(primCount);

                        for (auto &prim : mesh.primitives)
                        {
                                s->readPOD(prim.mode);
                                s->readPOD(prim.materialHandle);
                                s->readPOD(prim.attributeFlags);
                                s->readPOD(prim.vertexStride);
                                s->readPOD(prim.vertexCount);
                                s->readPOD(prim.bounds.min);
                                s->readPOD(prim.bounds.max);

                                uint32_t morphCount = 0;
                                s->readPOD(morphCount);
                                prim.morphWeights.resize(morphCount);
                                if (morphCount > 0)
                                        s->read(prim.morphWeights.data(), morphCount * sizeof(float));

                                uint64_t vbBytes = 0;
                                s->readPOD(vbBytes);
                                prim.vertexData.resize(static_cast<size_t>(vbBytes));
                                if (vbBytes > 0)
                                        s->read(prim.vertexData.data(), static_cast<size_t>(vbBytes));

                                uint32_t idxCount = 0;
                                s->readPOD(idxCount);
                                prim.indices.resize(idxCount);
                                if (idxCount > 0)
                                        s->read(prim.indices.data(), idxCount * sizeof(uint32_t));
                        }
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [meshes] end   @ {} ({} meshes)", s->tell(), meshCount);
#endif
        }

        // -----------------------------------------------------------------------
        // World bounds
        // -----------------------------------------------------------------------
        s->readPOD(m_worldBounds.min);
        s->readPOD(m_worldBounds.max);

        m_state = State::CPUReady;

#ifndef NDEBUG
        IC_CORE_TRACE("serializedLoad END @ {} bytes - {} meshes {} nodes", s->tell(), m_meshes.size(), m_nodes.size());
#endif

        return true;
}

}  // namespace ic