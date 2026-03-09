#include "core/assets/asset_loaders/gltf_loader.h"
#include "core/assets/types/model.h"

#include <filesystem>
#include <fstream>
#include <variant>
#include <algorithm>
#include <cstring>
#include <numeric>

#include <glm/gtx/matrix_decompose.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ic
{
/**
 * gltf_loader.cpp
 *
 * Implementation notes:
 *
 *  Load order matters -> GLTF indices are positional:
 *    buffers → bufferViews → accessors   (intermediate layer)
 *    samplers → images → textures        (resource layer, order-dependent)
 *    materials → nodes → meshes → scenes (scene-graph layer)
 *    animations / skins                  (reference nodes, so load after)
 *
 *  Geometry is a two-pass process:
 *    Pass 1 (loadMesh):          record accessor indices into m_tempPrimitiveData
 *    Pass 2 (processMeshGeometry): resolve accessors → fill vertices/indices
 *  This avoids re-iterating the fastgltf mesh list a second time and keeps
 *  the accessor resolution logic in one place.
 *
 *  All accessor indirection is resolved inside this file.
 *  Nothing that references Buffer / BufferView / Accessor escapes to the caller.
 *  freeIntermediates() is called before load() returns.
 */

static inline Index toIdx(size_t v)
{
        return static_cast<Index>(v);
}

/** Number of scalar components for a given Accessor::Type */
static size_t accessorComponentCount(Accessor::Type type)
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

static Accessor::Type mapAccessorType(fastgltf::AccessorType t)
{
        switch (t)
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

/** Extract a fastgltf AccessorBoundsArray into a plain vector<double> */
static std::vector<double> extractBounds(const fastgltf::AccessorBoundsArray &bounds)
{
        std::vector<double> out(bounds.size());
        if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::float64)
        {
                const double *d = bounds.data<double>();
                std::copy(d, d + bounds.size(), out.begin());
        }
        else if (bounds.type() == fastgltf::AccessorBoundsArray::BoundsType::int64)
        {
                const int64_t *d = bounds.data<int64_t>();
                std::transform(d, d + bounds.size(), out.begin(), [](int64_t v) { return static_cast<double>(v); });
        }
        return out;
}

/** Read a vec3 from accessor min/max bounds (for AABB extraction) */
static glm::vec3 boundsToVec3(const std::vector<double> &v, glm::vec3 fallback = glm::vec3(0.f))
{
        if (v.size() < 3)
                return fallback;
        return glm::vec3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
}

bool GLTFLoader::canLoad(const char *ext) const
{
        if (!ext)
                return false;
        return (strcmp(ext, "gltf") == 0 || strcmp(ext, "glb") == 0);
}

bool GLTFLoader::load(const char *path, ModelImportData *out)
{
        IC_CORE_ASSERT(path, "GLTFLoader::load -> null path");

        if (!fs_exists(path))
        {
                IC_CORE_WARN("GLTFLoader: file not found: {}", path);
                return false;
        }
        const char *fullpath = fs_getfullpath(path);

        IC_CORE_INFO("GLTFLoader: loading {}", fullpath);

        // -----------------------------------------------------------------------
        // Configure fastgltf parser
        // -----------------------------------------------------------------------
        static constexpr auto kExtensions = fastgltf::Extensions::KHR_mesh_quantization |
                                            fastgltf::Extensions::KHR_texture_transform |
                                            fastgltf::Extensions::KHR_materials_variants;

        static constexpr auto kOptions = fastgltf::Options::DontRequireValidAssetMember |
                                         fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers |
                                         fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;

        fastgltf::Parser parser(kExtensions);

        auto gltfFile = fastgltf::MappedGltfFile::FromPath(std::filesystem::path(fullpath));
        if (!bool(gltfFile))
        {
                IC_CORE_WARN("GLTFLoader: failed to open file: {}", fastgltf::getErrorMessage(gltfFile.error()));
                return false;
        }

        const char *parentPath    = fs_getParentPath(fullpath);
        auto        expectedAsset = parser.loadGltf(gltfFile.get(), std::filesystem::path(parentPath), kOptions);

        if (expectedAsset.error() != fastgltf::Error::None)
        {
                IC_CORE_WARN("GLTFLoader: parse error: {} (dir: {})",
                             fastgltf::getErrorMessage(expectedAsset.error()),
                             parentPath);
                return false;
        }

        fastgltf::Asset *asset = expectedAsset.get_if();
        if (!asset)
        {
                IC_CORE_WARN("GLTFLoader: failed to obtain asset handle");
                return false;
        }

        out->sourceFormat = "gltf";

        bool ok = parseAsset(asset, parentPath, out);

        // Always clear per-load temp state, even on failure
        m_tempPrimitiveData.clear();
        ic_free((void *)fullpath);

        return ok;
}

bool GLTFLoader::parseAsset(fastgltf::Asset *asset, const char *basePath, ModelImportData *out)
{
        // --- Intermediate buffer layer (must come first) ---
        out->buffers.reserve(asset->buffers.size());
        for (const auto &b : asset->buffers)
                loadBuffer(asset, &b, basePath, out);

        out->bufferViews.reserve(asset->bufferViews.size());
        for (const auto &bv : asset->bufferViews)
                loadBufferView(&bv, out);

        out->accessors.reserve(asset->accessors.size());
        for (const auto &acc : asset->accessors)
                loadAccessor(&acc, out);

        // --- Resource layer ---
        out->samplers.reserve(asset->samplers.size());
        for (const auto &s : asset->samplers)
                loadSampler(&s, out);

        out->images.reserve(asset->images.size());
        for (const auto &img : asset->images)
        {
                if (!loadImage(asset, &img, out))
                {
                        IC_CORE_WARN("GLTFLoader: image load failed, inserting placeholder");
                        out->images.push_back({});  // keep indices consistent
                }
        }

        out->textures.reserve(asset->textures.size());
        for (const auto &tex : asset->textures)
                loadTexture(&tex, out);

        out->materials.reserve(asset->materials.size());
        for (const auto &mat : asset->materials)
                loadMaterial(&mat, out);

        // --- Scene-graph layer ---
        out->nodes.reserve(asset->nodes.size());
        for (const auto &node : asset->nodes)
                loadNode(&node, out);

        out->cameras.reserve(asset->cameras.size());
        for (const auto &cam : asset->cameras)
                loadCamera(&cam, out);

        out->skins.reserve(asset->skins.size());
        for (const auto &skin : asset->skins)
                loadSkin(asset, &skin, out);

        // --- Mesh layer (pass 1 -> records accessor indices) ---
        out->meshes.reserve(asset->meshes.size());
        m_tempPrimitiveData.reserve(std::accumulate(asset->meshes.begin(),
                                                    asset->meshes.end(),
                                                    size_t(0),
                                                    [](size_t n, const fastgltf::Mesh &m)
                                                    { return n + m.primitives.size(); }));

        for (const auto &mesh : asset->meshes)
                loadMesh(&mesh, out);

        // --- Scene layer ---
        out->scenes.reserve(asset->scenes.size());
        for (auto &scene : asset->scenes)
                loadScene(&scene, out);

        // --- Animation layer (fully decodes keyframes from accessors) ---
        out->animations.reserve(asset->animations.size());
        for (auto &anim : asset->animations)
                loadAnimation(asset, &anim, out);

        // --- Default scene ---
        if (asset->defaultScene.has_value())
                out->defaultScene = toIdx(asset->defaultScene.value());
        else if (!out->scenes.empty())
                out->defaultScene = 0;
        else
                out->defaultScene = INVALID_INDEX;

        // --- Mesh layer pass 2 -> geometry extraction ---
        processMeshGeometry(out);

        // --- Free buffer intermediates -> nothing downstream needs them ---
        out->freeIntermediates();

        IC_CORE_TRACE("GLTFLoader: {} scenes, {} nodes, {} meshes, {} materials, {} images, {} animations",
                      out->scenes.size(),
                      out->nodes.size(),
                      out->meshes.size(),
                      out->materials.size(),
                      out->images.size(),
                      out->animations.size());

        return true;
}

void GLTFLoader::loadBuffer(fastgltf::Asset * /*asset*/,
                            const fastgltf::Buffer *src,
                            const char             *basePath,
                            ModelImportData        *out)
{
        Buffer buf;

        std::visit(fastgltf::visitor{[&](const fastgltf::sources::Array &arr)
                                     {
                                             buf.data.resize(arr.bytes.size());
                                             memcpy(buf.data.data(), arr.bytes.data(), arr.bytes.size());
                                     },
                                     [&](const fastgltf::sources::Vector &vec)
                                     {
                                             buf.data.resize(vec.bytes.size());
                                             memcpy(buf.data.data(), vec.bytes.data(), vec.bytes.size());
                                     },
                                     [&](const fastgltf::sources::ByteView &bv)
                                     {
                                             buf.data.resize(bv.bytes.size());
                                             memcpy(buf.data.data(), bv.bytes.data(), bv.bytes.size());
                                     },
                                     [&](const fastgltf::sources::URI &uri)
                                     {
                                             const char *bufPath = nullptr;
                                             fs_joinPath(uri.uri.path().data(), basePath, &bufPath);

                                             std::ifstream file(bufPath, std::ios::binary | std::ios::ate);
                                             if (!file)
                                             {
                                                     IC_CORE_ERROR("GLTFLoader: cannot open buffer file: {}", bufPath);
                                                     return;
                                             }
                                             size_t size = static_cast<size_t>(file.tellg());
                                             file.seekg(0);
                                             buf.data.resize(size);
                                             file.read(reinterpret_cast<char *>(buf.data.data()),
                                                       static_cast<std::streamsize>(size));
                                     },
                                     [&](auto &&) { IC_CORE_WARN("GLTFLoader: unsupported buffer source type"); }},
                   src->data);

        out->buffers.push_back(std::move(buf));
}

void GLTFLoader::loadBufferView(const fastgltf::BufferView *src, ModelImportData *out)
{
        BufferView bv;
        bv.bufferIndex = toIdx(src->bufferIndex);
        bv.byteOffset  = src->byteOffset;
        bv.byteLength  = src->byteLength;
        bv.byteStride  = src->byteStride.value_or(0);
        bv.name        = src->name;
        out->bufferViews.push_back(std::move(bv));
}

void GLTFLoader::loadAccessor(const fastgltf::Accessor *src, ModelImportData *out)
{
        Accessor acc;
        acc.bufferView    = src->bufferViewIndex.has_value() ? toIdx(src->bufferViewIndex.value()) : INVALID_INDEX;
        acc.offset        = src->byteOffset;
        acc.count         = src->count;
        acc.type          = mapAccessorType(src->type);
        acc.componentType = static_cast<Accessor::ComponentType>(src->componentType);
        acc.normalized    = src->normalized;

        if (src->min.has_value())
                acc.min = extractBounds(src->min.value());
        if (src->max.has_value())
                acc.max = extractBounds(src->max.value());

        out->accessors.push_back(std::move(acc));
}

void GLTFLoader::loadSampler(const fastgltf::Sampler *src, ModelImportData *out)
{
        SamplerImportData s;
        if (src->magFilter.has_value())
                s.magFilter = static_cast<SamplerImportData::Filter>(src->magFilter.value());
        if (src->minFilter.has_value())
                s.minFilter = static_cast<SamplerImportData::Filter>(src->minFilter.value());
        s.wrapS = static_cast<SamplerImportData::Wrap>(src->wrapS);
        s.wrapT = static_cast<SamplerImportData::Wrap>(src->wrapT);
        out->samplers.push_back(std::move(s));
}

bool GLTFLoader::loadImage(fastgltf::Asset *asset, const fastgltf::Image *src, ModelImportData *out)
{
        ImageImportData img;
        img.name = src->name;

        bool decoded = false;

        auto decodePixels = [&](const stbi_uc *data, int len) -> bool
        {
                int      w, h, ch;
                uint8_t *pixels = stbi_load_from_memory(data, len, &w, &h, &ch, 4);
                if (!pixels)
                {
                        IC_CORE_ERROR("GLTFLoader: stb_image decode failed: {}", stbi_failure_reason());
                        return false;
                }
                img.width    = static_cast<uint32_t>(w);
                img.height   = static_cast<uint32_t>(h);
                img.channels = static_cast<uint32_t>(ch);
                size_t size  = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
                img.pixels.assign(pixels, pixels + size);
                stbi_image_free(pixels);
                return true;
        };

        std::visit(
            fastgltf::visitor{
                [&](const fastgltf::sources::URI &uri)
                {
                        IC_CORE_ASSERT(uri.fileByteOffset == 0, "GLTFLoader: non-zero image byte offset unsupported");
                        IC_CORE_ASSERT(uri.uri.isLocalPath(), "GLTFLoader: non-local image URI unsupported");

                        std::string path(uri.uri.path().begin(), uri.uri.path().end());
                        img.uri = path;

                        int      w, h, ch;
                        uint8_t *pixels = stbi_load(path.c_str(), &w, &h, &ch, 4);
                        if (!pixels)
                        {
                                IC_CORE_ERROR("GLTFLoader: failed to load image file: {}", path);
                                return;
                        }
                        img.width    = static_cast<uint32_t>(w);
                        img.height   = static_cast<uint32_t>(h);
                        img.channels = static_cast<uint32_t>(ch);
                        size_t size  = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
                        img.pixels.assign(pixels, pixels + size);
                        stbi_image_free(pixels);
                        decoded = true;
                },
                [&](const fastgltf::sources::Array &arr)
                {
                        decoded = decodePixels(reinterpret_cast<const stbi_uc *>(arr.bytes.data()),
                                               static_cast<int>(arr.bytes.size()));
                },
                [&](const fastgltf::sources::BufferView &bvSrc)
                {
                        // Image embedded in a buffer view -> need to reach into fastgltf's asset buffers
                        const auto &bv  = asset->bufferViews[bvSrc.bufferViewIndex];
                        const auto &buf = asset->buffers[bv.bufferIndex];

                        std::visit(fastgltf::visitor{[&](const fastgltf::sources::Array &arr)
                                                     {
                                                             decoded = decodePixels(reinterpret_cast<const stbi_uc *>(
                                                                                        arr.bytes.data() +
                                                                                        bv.byteOffset),
                                                                                    static_cast<int>(bv.byteLength));
                                                     },
                                                     [&](auto &&)
                                                     {
                                                             IC_CORE_WARN("GLTFLoader: unsupported buffer source for "
                                                                          "embedded image");
                                                     }},
                                   buf.data);
                },
                [&](auto &&) { IC_CORE_WARN("GLTFLoader: unsupported image source type for '{}'", src->name); }},
            src->data);

        out->images.push_back(std::move(img));
        return decoded;
}

void GLTFLoader::loadTexture(const fastgltf::Texture *src, ModelImportData *out)
{
        TextureImportData tex;
        if (src->imageIndex.has_value())
                tex.image = toIdx(src->imageIndex.value());
        if (src->samplerIndex.has_value())
                tex.sampler = toIdx(src->samplerIndex.value());
        out->textures.push_back(std::move(tex));
}

void GLTFLoader::loadMaterial(const fastgltf::Material *src, ModelImportData *out)
{
        MaterialImportData mat;
        mat.name = src->name;

        // PBR metallic-roughness
        {
                const auto &pbr         = src->pbrData;
                const auto &bcf         = pbr.baseColorFactor;
                mat.pbr.baseColorFactor = glm::vec4(bcf[0], bcf[1], bcf[2], bcf[3]);
                mat.pbr.metallicFactor  = pbr.metallicFactor;
                mat.pbr.roughnessFactor = pbr.roughnessFactor;

                if (pbr.baseColorTexture.has_value())
                {
                        mat.pbr.baseColorTexture.idx      = toIdx(pbr.baseColorTexture->textureIndex);
                        mat.pbr.baseColorTexture.texCoord = toIdx(pbr.baseColorTexture->texCoordIndex);
                }
                if (pbr.metallicRoughnessTexture.has_value())
                {
                        mat.pbr.metallicRoughnessTexture.idx      = toIdx(pbr.metallicRoughnessTexture->textureIndex);
                        mat.pbr.metallicRoughnessTexture.texCoord = toIdx(pbr.metallicRoughnessTexture->texCoordIndex);
                }
        }

        // Normal texture
        if (src->normalTexture.has_value())
        {
                mat.normalTexture.ref.idx      = toIdx(src->normalTexture->textureIndex);
                mat.normalTexture.ref.texCoord = toIdx(src->normalTexture->texCoordIndex);
                mat.normalTexture.scale        = src->normalTexture->scale;
        }

        // Occlusion texture
        if (src->occlusionTexture.has_value())
        {
                mat.occlusionTexture.ref.idx      = toIdx(src->occlusionTexture->textureIndex);
                mat.occlusionTexture.ref.texCoord = toIdx(src->occlusionTexture->texCoordIndex);
                mat.occlusionTexture.strength     = src->occlusionTexture->strength;
        }

        // Emissive
        if (src->emissiveTexture.has_value())
        {
                mat.emissiveTexture.idx      = toIdx(src->emissiveTexture->textureIndex);
                mat.emissiveTexture.texCoord = toIdx(src->emissiveTexture->texCoordIndex);
        }
        const auto &emm    = src->emissiveFactor;
        mat.emissiveFactor = glm::vec3(emm[0], emm[1], emm[2]);

        mat.alphaCutoff = src->alphaCutoff;
        mat.alphaMode   = static_cast<MaterialImportData::AlphaMode>(src->alphaMode);
        mat.doubleSided = src->doubleSided;

        out->materials.push_back(std::move(mat));
}

void GLTFLoader::loadNode(const fastgltf::Node *src, ModelImportData *out)
{
        NodeImportData node;
        node.name = src->name;

        if (src->meshIndex.has_value())
                node.meshIndex = toIdx(src->meshIndex.value());
        if (src->skinIndex.has_value())
                node.skinIndex = toIdx(src->skinIndex.value());
        if (src->cameraIndex.has_value())
                node.cameraIndex = toIdx(src->cameraIndex.value());
        if (src->lightIndex.has_value())
                node.lightIndex = toIdx(src->lightIndex.value());

        node.children.reserve(src->children.size());
        for (size_t childIdx : src->children)
                node.children.push_back(toIdx(childIdx));

        if (std::holds_alternative<fastgltf::TRS>(src->transform))
        {
                const auto &trs  = std::get<fastgltf::TRS>(src->transform);
                node.translation = glm::vec3(trs.translation[0], trs.translation[1], trs.translation[2]);
                node.rotation    = glm::quat(trs.rotation[3],  // GLM: w first
                                          trs.rotation[0],
                                          trs.rotation[1],
                                          trs.rotation[2]);
                node.scale       = glm::vec3(trs.scale[0], trs.scale[1], trs.scale[2]);

                // Build local matrix from TRS
                glm::mat4 m         = glm::mat4(1.0f);
                m                   = glm::translate(m, node.translation);
                m                   = m * glm::mat4_cast(node.rotation);
                m                   = glm::scale(m, node.scale);
                node.localTransform = m;
        }
        else if (std::holds_alternative<fastgltf::math::fmat4x4>(src->transform))
        {
                const auto &fm = std::get<fastgltf::math::fmat4x4>(src->transform);
                // fastgltf is column-major, GLM is column-major -> direct copy
                memcpy(&node.localTransform[0][0], fm.data(), 16 * sizeof(float));

                // Decompose for TRS fields so ModelBuilder can animate them
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(node.localTransform, node.scale, node.rotation, node.translation, skew, perspective);
        }

        out->nodes.push_back(std::move(node));
}

void GLTFLoader::loadCamera(const fastgltf::Camera *src, ModelImportData *out)
{
        CameraImportData cam;
        cam.name = src->name;

        std::visit(fastgltf::visitor{[&](const fastgltf::Camera::Perspective &p)
                                     {
                                             cam.type                    = CameraImportData::Type::Perspective;
                                             cam.perspective.yfov        = p.yfov;
                                             cam.perspective.znear       = p.znear;
                                             cam.perspective.zfar        = p.zfar.value_or(0.0f);
                                             cam.perspective.aspectRatio = p.aspectRatio.value_or(0.0f);
                                     },
                                     [&](const fastgltf::Camera::Orthographic &o)
                                     {
                                             cam.type               = CameraImportData::Type::Orthographic;
                                             cam.orthographic.xmag  = o.xmag;
                                             cam.orthographic.ymag  = o.ymag;
                                             cam.orthographic.zfar  = o.zfar;
                                             cam.orthographic.znear = o.znear;
                                     }},
                   src->camera);

        out->cameras.push_back(std::move(cam));
}

void GLTFLoader::loadSkin(fastgltf::Asset *asset, const fastgltf::Skin *src, ModelImportData *out)
{
        SkinImportData skin;
        skin.name = src->name;

        skin.jointIndices.reserve(src->joints.size());
        for (size_t ji : src->joints)
                skin.jointIndices.push_back(toIdx(ji));

        if (src->skeleton.has_value())
                skin.skeletonRootIndex = toIdx(src->skeleton.value());

        // Decode inverse bind matrices directly from the accessor
        if (src->inverseBindMatrices.has_value())
        {
                Index ibmIdx = toIdx(src->inverseBindMatrices.value());
                readAccessorMat4(ibmIdx, skin.inverseBindMatrices, out);
        }
        else
        {
                // Default: identity matrices for all joints
                skin.inverseBindMatrices.resize(src->joints.size(), glm::mat4(1.0f));
        }

        out->skins.push_back(std::move(skin));
}

void GLTFLoader::loadScene(const fastgltf::Scene *src, ModelImportData *out)
{
        SceneImportData scene;
        scene.name = src->name;
        scene.rootNodes.reserve(src->nodeIndices.size());
        for (size_t ni : src->nodeIndices)
                scene.rootNodes.push_back(toIdx(ni));
        out->scenes.push_back(std::move(scene));
}

void GLTFLoader::loadMesh(const fastgltf::Mesh *src, ModelImportData *out)
{
        MeshImportData mesh;
        mesh.name = src->name;
        mesh.primitives.reserve(src->primitives.size());

        for (const auto &prim : src->primitives)
        {
                MeshPrimitiveImportData outPrim;
                outPrim.mode = static_cast<MeshPrimitiveImportData::Mode>(prim.type);

                if (prim.materialIndex.has_value())
                        outPrim.materialIndex = toIdx(prim.materialIndex.value());

                // Record morph weights
                outPrim.morphWeights.assign(src->weights.begin(), src->weights.end());

                // Build accessor index cache for pass 2
                TempPrimitiveData temp;

                auto posIt = prim.findAttribute("POSITION");
                IC_CORE_ASSERT(posIt != prim.attributes.end(),
                               "GLTFLoader: mesh '{}' primitive missing POSITION attribute",
                               src->name);
                temp.positionAccessor = toIdx(posIt->accessorIndex);

                for (const auto &[attrName, accessorIndex] : prim.attributes)
                {
                        if (attrName == "NORMAL")
                                temp.normalAccessor = toIdx(accessorIndex);
                        else if (attrName == "TANGENT")
                                temp.tangentAccessor = toIdx(accessorIndex);
                        else if (attrName == "COLOR_0")
                                temp.colorAccessor = toIdx(accessorIndex);
                        else if (attrName == "TEXCOORD_0")
                                temp.texCoord0Accessor = toIdx(accessorIndex);
                        else if (attrName == "TEXCOORD_1")
                                temp.texCoord1Accessor = toIdx(accessorIndex);
                        else if (attrName == "TEXCOORD_2")
                                temp.texCoord2Accessor = toIdx(accessorIndex);
                        else if (attrName == "JOINTS_0")
                                temp.jointsAccessor = toIdx(accessorIndex);
                        else if (attrName == "WEIGHTS_0")
                                temp.weightsAccessor = toIdx(accessorIndex);
                }

                if (prim.indicesAccessor.has_value())
                        temp.indicesAccessor = toIdx(prim.indicesAccessor.value());

                m_tempPrimitiveData.push_back(std::move(temp));
                mesh.primitives.push_back(std::move(outPrim));
        }

        out->meshes.push_back(std::move(mesh));
}

void GLTFLoader::loadAnimation(fastgltf::Asset * /*asset*/, const fastgltf::Animation *src, ModelImportData *out)
{
        AnimationImportData anim;
        anim.name = src->name;

        // Decode samplers -> resolve both input (times) and output (values) accessors
        anim.samplers.reserve(src->samplers.size());
        for (const auto &s : src->samplers)
        {
                AnimationSamplerImportData sampler;

                switch (s.interpolation)
                {
                case fastgltf::AnimationInterpolation::Linear:
                        sampler.interpolation = AnimationSamplerImportData::Interpolation::Linear;
                        break;
                case fastgltf::AnimationInterpolation::Step:
                        sampler.interpolation = AnimationSamplerImportData::Interpolation::Step;
                        break;
                case fastgltf::AnimationInterpolation::CubicSpline:
                        sampler.interpolation = AnimationSamplerImportData::Interpolation::CubicSpline;
                        break;
                }

                // Input times -> always float scalars
                readAccessorFloat(toIdx(s.inputAccessor), sampler.inputTimes, out);

                // Update animation duration from this sampler's max input time
                if (!sampler.inputTimes.empty())
                        anim.duration = std::max(anim.duration, sampler.inputTimes.back());

                // Output values -> type depends on the target channel, but we store as vec4
                // Translation → vec3 (w=0), Rotation → quat/vec4, Scale → vec3 (w=1), Weights → float
                // We read vec4 for rotation, and vec3 padded for the rest during channel processing.
                // For now, read raw as vec4 -> the runtime animator unpacks based on channel path.
                readAccessorVec4(toIdx(s.outputAccessor), sampler.outputValues, out);

                anim.samplers.push_back(std::move(sampler));
        }

        // Decode channels
        anim.channels.reserve(src->channels.size());
        for (const auto &ch : src->channels)
        {
                AnimationChannelImportData channel;
                channel.samplerIndex = toIdx(ch.samplerIndex);

                if (ch.nodeIndex.has_value())
                        channel.targetNodeIndex = toIdx(ch.nodeIndex.value());

                switch (ch.path)
                {
                case fastgltf::AnimationPath::Translation:
                        channel.targetPath = AnimationChannelImportData::Path::Translation;
                        break;
                case fastgltf::AnimationPath::Rotation:
                        channel.targetPath = AnimationChannelImportData::Path::Rotation;
                        break;
                case fastgltf::AnimationPath::Scale:
                        channel.targetPath = AnimationChannelImportData::Path::Scale;
                        break;
                case fastgltf::AnimationPath::Weights:
                        channel.targetPath = AnimationChannelImportData::Path::Weights;
                        break;
                }

                anim.channels.push_back(channel);
        }

        out->animations.push_back(std::move(anim));
}

void GLTFLoader::processMeshGeometry(ModelImportData *out)
{
        size_t tempIdx = 0;

        for (auto &mesh : out->meshes)
        {
                // Track mesh-level AABB -> start with inverted infinity bounds
                glm::vec3 meshMin(std::numeric_limits<float>::max());
                glm::vec3 meshMax(-std::numeric_limits<float>::max());

                for (auto &prim : mesh.primitives)
                {
                        if (tempIdx >= m_tempPrimitiveData.size())
                        {
                                IC_CORE_ERROR("GLTFLoader: temp primitive index out of range");
                                return;
                        }

                        const TempPrimitiveData *temp = &m_tempPrimitiveData[tempIdx++];

                        extractVertices(temp, &prim, out);
                        extractIndices(temp, &prim, out);

                        // Merge primitive AABB up to mesh AABB
                        meshMin = glm::min(meshMin, prim.aabbMin);
                        meshMax = glm::max(meshMax, prim.aabbMax);
                }

                mesh.aabbMin = meshMin;
                mesh.aabbMax = meshMax;
        }
}

void GLTFLoader::extractVertices(const TempPrimitiveData *temp, MeshPrimitiveImportData *prim, ModelImportData *out)
{
        if (temp->positionAccessor == INVALID_INDEX)
                return;

        const Accessor &posAcc   = out->accessors[temp->positionAccessor];
        const size_t    vtxCount = posAcc.count;
        prim->vertices.resize(vtxCount);

        // --- Positions (required) ---
        {
                std::vector<glm::vec3> positions;
                readAccessorVec3(temp->positionAccessor, positions, out);
                for (size_t i = 0; i < vtxCount; ++i)
                        prim->vertices[i].pos = positions[i];
        }

        // --- AABB from accessor min/max -> avoids iterating vertices again ---
        prim->aabbMin = boundsToVec3(posAcc.min, glm::vec3(std::numeric_limits<float>::max()));
        prim->aabbMax = boundsToVec3(posAcc.max, glm::vec3(-std::numeric_limits<float>::max()));

        // --- Normals ---
        if (temp->normalAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec3> normals;
                readAccessorVec3(temp->normalAccessor, normals, out);
                for (size_t i = 0; i < std::min(vtxCount, normals.size()); ++i)
                        prim->vertices[i].normal = normals[i];
        }

        // --- Tangents ---
        if (temp->tangentAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> tangents;
                readAccessorVec4(temp->tangentAccessor, tangents, out);
                for (size_t i = 0; i < std::min(vtxCount, tangents.size()); ++i)
                        prim->vertices[i].tangent = tangents[i];
        }

        // --- UVs ---
        auto readUV = [&](Index accIdx, auto memberPtr)
        {
                if (accIdx == INVALID_INDEX)
                        return;
                std::vector<glm::vec2> uvs;
                readAccessorVec2(accIdx, uvs, out);
                for (size_t i = 0; i < std::min(vtxCount, uvs.size()); ++i)
                        prim->vertices[i].*memberPtr = uvs[i];
        };
        readUV(temp->texCoord0Accessor, &Vertex::uv0);
        readUV(temp->texCoord1Accessor, &Vertex::uv1);
        readUV(temp->texCoord2Accessor, &Vertex::uv2);

        // --- Vertex colors ---
        if (temp->colorAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> colors;
                readAccessorVec4(temp->colorAccessor, colors, out);
                for (size_t i = 0; i < std::min(vtxCount, colors.size()); ++i)
                        prim->vertices[i].color = colors[i];
        }

        // --- Skinning ---
        if (temp->jointsAccessor != INVALID_INDEX)
        {
                std::vector<glm::uvec4> joints;
                readAccessorUVec4(temp->jointsAccessor, joints, out);
                for (size_t i = 0; i < std::min(vtxCount, joints.size()); ++i)
                        prim->vertices[i].joint0 = joints[i];
        }

        if (temp->weightsAccessor != INVALID_INDEX)
        {
                std::vector<glm::vec4> weights;
                readAccessorVec4(temp->weightsAccessor, weights, out);
                for (size_t i = 0; i < std::min(vtxCount, weights.size()); ++i)
                        prim->vertices[i].weight0 = weights[i];
        }
}

void GLTFLoader::extractIndices(const TempPrimitiveData *temp, MeshPrimitiveImportData *prim, ModelImportData *out)
{
        if (temp->indicesAccessor == INVALID_INDEX)
                return;

        const Accessor &acc = out->accessors[temp->indicesAccessor];
        prim->indices.resize(acc.count);

        const uint8_t *data = getAccessorData(temp->indicesAccessor, out);
        if (!data)
        {
                IC_CORE_ERROR("GLTFLoader: null accessor data for index buffer");
                return;
        }

        switch (acc.componentType)
        {
        case Accessor::ComponentType::UByte:
        {
                const uint8_t *src = data;
                for (size_t i = 0; i < acc.count; ++i)
                        prim->indices[i] = static_cast<uint32_t>(src[i]);
                break;
        }
        case Accessor::ComponentType::UShort:
        {
                const uint16_t *src = reinterpret_cast<const uint16_t *>(data);
                for (size_t i = 0; i < acc.count; ++i)
                        prim->indices[i] = static_cast<uint32_t>(src[i]);
                break;
        }
        case Accessor::ComponentType::UInt:
        {
                memcpy(prim->indices.data(), data, acc.count * sizeof(uint32_t));
                break;
        }
        default:
                IC_CORE_ERROR("GLTFLoader: unsupported index component type");
                break;
        }
}

const uint8_t *GLTFLoader::getAccessorData(Index accessorIndex, const ModelImportData *out) const
{
        if (accessorIndex >= out->accessors.size())
                return nullptr;
        const Accessor &acc = out->accessors[accessorIndex];

        if (acc.bufferView >= out->bufferViews.size())
                return nullptr;
        const BufferView &bv = out->bufferViews[acc.bufferView];

        if (bv.bufferIndex >= out->buffers.size())
                return nullptr;
        const Buffer &buf = out->buffers[bv.bufferIndex];

        return buf.data.data() + bv.byteOffset + acc.offset;
}

void GLTFLoader::readAccessorFloat(Index idx, std::vector<float> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const float    *data = reinterpret_cast<const float *>(getAccessorData(idx, out));
        outVec.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
                outVec[i] = data[i];
}

void GLTFLoader::readAccessorVec2(Index idx, std::vector<glm::vec2> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const float    *data = reinterpret_cast<const float *>(getAccessorData(idx, out));
        outVec.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
                outVec[i] = glm::vec2(data[i * 2], data[i * 2 + 1]);
}

void GLTFLoader::readAccessorVec3(Index idx, std::vector<glm::vec3> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const float    *data = reinterpret_cast<const float *>(getAccessorData(idx, out));
        outVec.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
                outVec[i] = glm::vec3(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
}

void GLTFLoader::readAccessorVec4(Index idx, std::vector<glm::vec4> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const float    *data = reinterpret_cast<const float *>(getAccessorData(idx, out));
        outVec.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
                outVec[i] = glm::vec4(data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
}

void GLTFLoader::readAccessorMat4(Index idx, std::vector<glm::mat4> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const float    *data = reinterpret_cast<const float *>(getAccessorData(idx, out));
        outVec.resize(acc.count);
        for (size_t i = 0; i < acc.count; ++i)
                memcpy(&outVec[i][0][0], data + i * 16, 16 * sizeof(float));
}

void GLTFLoader::readAccessorUVec4(Index idx, std::vector<glm::uvec4> &outVec, const ModelImportData *out) const
{
        const Accessor &acc  = out->accessors[idx];
        const uint8_t  *data = getAccessorData(idx, out);
        outVec.resize(acc.count);

        // Joint indices are stored as UNSIGNED_BYTE or UNSIGNED_SHORT per spec
        switch (acc.componentType)
        {
        case Accessor::ComponentType::UByte:
        {
                for (size_t i = 0; i < acc.count; ++i)
                        outVec[i] = glm::uvec4(data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
                break;
        }
        case Accessor::ComponentType::UShort:
        {
                const uint16_t *src = reinterpret_cast<const uint16_t *>(data);
                for (size_t i = 0; i < acc.count; ++i)
                        outVec[i] = glm::uvec4(src[i * 4], src[i * 4 + 1], src[i * 4 + 2], src[i * 4 + 3]);
                break;
        }
        default:
                IC_CORE_ERROR("GLTFLoader: unsupported component type for JOINTS accessor");
                break;
        }
}

}  // namespace ic
