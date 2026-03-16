#include "core/assets/types/model.h"
#include "core/assets/asset_serializer.h"
#include "core/assets/asset_manager.h"
#include "core/assets/asset_versions.h"
#include "core/assets/asset_cache.h"
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
        const char *cachePath = AssetManager::Get().GetRegistry()->GetCachePath(getID());
        if (cachePath && fs_exists(cachePath))
        {
                uint64_t srcTime   = fs_getLastModificationTime(filepath);
                uint64_t cacheTime = AssetCache::GetAssetTimeStamp(AssetType::ASSET_TYPE_MODEL, getID());

                if (cacheTime >= srcTime)
                {
                        IC_CORE_INFO("Model::load -> Loading cached model from memory '{}'", filepath);
                        ic::Serializer s;
#ifndef NDEBUG
                        auto start = std::chrono::high_resolution_clock::now();
#endif
                        if (s.openForRead(cachePath) && serializedLoad(&s))
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

        *this   = buildModel(&importData, getID());
        m_state = State::CPUReady;

        // -----------------------------------------------------------------------
        // Write cache for next time
        // -----------------------------------------------------------------------
        if (!AssetCache::CacheAsset(AssetType::ASSET_TYPE_MODEL, getID(), this))
                IC_CORE_WARN("Model::load -> failed to write cache for '{}'", filepath);

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

bool Model::serializedSave(ic::Serializer *s) const
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
        // Images
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [images]  start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_images.size());
                s->writePOD(count);

                for (const auto &img : m_images)
                {
                        s->writePOD(img.width);
                        s->writePOD(img.height);
                        s->writePOD(img.channels);
                        s->writePOD(img.srgb);
#if defined(IC_ASSET_NAMES)
                        s->writeString(img.name);
#else
                        s->writeString("");  // placeholder - must always be written and read
#endif
                        uint64_t pixelBytes = static_cast<uint64_t>(img.pixels.size());
                        s->writePOD(pixelBytes);
                        if (pixelBytes > 0)
                                s->write(img.pixels.data(), static_cast<size_t>(pixelBytes));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [images]  end   @ {} ({} images)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Samplers
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [samplers] start @ {}", s->tell());
#endif
                uint32_t count = static_cast<uint32_t>(m_samplers.size());
                s->writePOD(count);

                for (const auto &samp : m_samplers)
                {
                        s->writePOD(samp.magFilter);
                        s->writePOD(samp.minFilter);
                        s->writePOD(samp.wrapS);
                        s->writePOD(samp.wrapT);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [samplers] end   @ {} ({} samplers)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Materials
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] start @ {}", s->tell());
#endif
                auto writeTextureRef = [&](const TextureRef &ref)
                {
                        s->writePOD(ref.image);
                        s->writePOD(ref.sampler);
                        s->writePOD(ref.texCoord);
                };

                uint32_t count = static_cast<uint32_t>(m_materials.size());
                s->writePOD(count);

                for (const auto &mat : m_materials)
                {
                        s->writeString(mat.name);
                        s->writePOD(mat.pbr.baseColorFactor);
                        s->writePOD(mat.pbr.metallicFactor);
                        s->writePOD(mat.pbr.roughnessFactor);
                        writeTextureRef(mat.pbr.baseColorTexture);
                        writeTextureRef(mat.pbr.metallicRoughnessTexture);
                        writeTextureRef(mat.normalTexture.ref);
                        s->writePOD(mat.normalTexture.scale);
                        writeTextureRef(mat.occlusionTexture.ref);
                        s->writePOD(mat.occlusionTexture.strength);
                        writeTextureRef(mat.emissiveTexture);
                        s->writePOD(mat.emissiveFactor);
                        s->writePOD(mat.emissiveStrength);
                        s->writePOD(mat.alphaMode);
                        s->writePOD(mat.alphaCutoff);
                        s->writePOD(mat.doubleSided);
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
                uint32_t count = static_cast<uint32_t>(m_cameras.size());
                s->writePOD(count);

                for (const auto &cam : m_cameras)
                {
                        s->writeString(cam.name);
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
                        s->writeString(skin.name);
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
                        s->writeString(anim.name);
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
                        s->writeString(node.name);
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
                        s->writeString(scene.name);
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
                        s->writeString(mesh.name);
                        s->writePOD(mesh.bounds.min);
                        s->writePOD(mesh.bounds.max);

                        uint32_t primCount = static_cast<uint32_t>(mesh.primitives.size());
                        s->writePOD(primCount);

                        for (const auto &prim : mesh.primitives)
                        {
                                s->writePOD(prim.mode);
                                s->writePOD(prim.materialIndex);
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

bool Model::serializedLoad(ic::Serializer *s)
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

        // -----------------------------------------------------------------------
        // Images
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [images]  start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_images.resize(count);

                for (auto &img : m_images)
                {
                        s->readPOD(img.width);
                        s->readPOD(img.height);
                        s->readPOD(img.channels);
                        s->readPOD(img.srgb);
                        s->readString(img.name);  // ALWAYS read - save always writes it

                        uint64_t pixelBytes = 0;
                        s->readPOD(pixelBytes);
                        img.pixels.resize(static_cast<size_t>(pixelBytes));
                        if (pixelBytes > 0)
                                s->read(img.pixels.data(), static_cast<size_t>(pixelBytes));
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [images]  end   @ {} ({} images)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Samplers
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [samplers] start @ {}", s->tell());
#endif
                uint32_t count = 0;
                s->readPOD(count);
                m_samplers.resize(count);

                for (auto &samp : m_samplers)
                {
                        s->readPOD(samp.magFilter);
                        s->readPOD(samp.minFilter);
                        s->readPOD(samp.wrapS);
                        s->readPOD(samp.wrapT);
                }
#ifndef NDEBUG
                IC_CORE_TRACE("  [samplers] end   @ {} ({} samplers)", s->tell(), count);
#endif
        }

        // -----------------------------------------------------------------------
        // Materials
        // -----------------------------------------------------------------------
        {
#ifndef NDEBUG
                IC_CORE_TRACE("  [materials] start @ {}", s->tell());
#endif
                auto readTextureRef = [&](TextureRef &ref)
                {
                        s->readPOD(ref.image);
                        s->readPOD(ref.sampler);
                        s->readPOD(ref.texCoord);
                };

                uint32_t count = 0;
                s->readPOD(count);
                m_materials.resize(count);

                for (auto &mat : m_materials)
                {
                        s->readString(mat.name);
                        s->readPOD(mat.pbr.baseColorFactor);
                        s->readPOD(mat.pbr.metallicFactor);
                        s->readPOD(mat.pbr.roughnessFactor);
                        readTextureRef(mat.pbr.baseColorTexture);
                        readTextureRef(mat.pbr.metallicRoughnessTexture);
                        readTextureRef(mat.normalTexture.ref);
                        s->readPOD(mat.normalTexture.scale);
                        readTextureRef(mat.occlusionTexture.ref);
                        s->readPOD(mat.occlusionTexture.strength);
                        readTextureRef(mat.emissiveTexture);
                        s->readPOD(mat.emissiveFactor);
                        s->readPOD(mat.emissiveStrength);
                        s->readPOD(mat.alphaMode);
                        s->readPOD(mat.alphaCutoff);
                        s->readPOD(mat.doubleSided);
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
                                s->readPOD(prim.materialIndex);
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
        IC_CORE_TRACE("serializedLoad END @ {} bytes - {} meshes {} images {} materials",
                      s->tell(),
                      m_meshes.size(),
                      m_images.size(),
                      m_materials.size());
#endif

        return true;
}

}  // namespace ic