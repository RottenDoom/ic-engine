#ifndef MODEL_H
#define MODEL_H

#include "defines.h"
#include "core/math.h"
#include "core/filesystem.h"

#include "core/assets/types/asset_base.h"
#include "material.h"

#include <vector>

/**
 * model.h -> Runtime Model asset.
 *
 * Build pipeline:
 *   Cold path:  filepath → IModelLoader → ModelImportData → ic::buildModel() → Model (CPUReady)
 *   Warm path:  .icache  → serializedLoad()                                  → Model (CPUReady)
 *   GPU upload: GLModel::upload(model) transitions Model to GPUReady
 *
 * Private data is written ONLY by ic::buildModel() and ic::Serializer.
 * No friend class -> a single friend free function limits access precisely.
 */

namespace ic
{

class Serializer;
struct ModelImportData;
class Model;

/**
 * Consumes import data and produces a CPUReady Model.
 * This is the only function that may write into Model's private members.
 * Defined in model_builder.cpp.
 */
Model build_model(ModelImportData *data, IC_GUID id);

// ---------------------------------------------------------------------------
// AABB
// ---------------------------------------------------------------------------

struct AABB
{
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

        bool      isValid() const { return min.x <= max.x; }
        glm::vec3 center() const { return (min + max) * 0.5f; }
        glm::vec3 extents() const { return (max - min) * 0.5f; }
        glm::vec3 size() const { return max - min; }

        void Expand(const glm::vec3 &p)
        {
                min = glm::min(min, p);
                max = glm::max(max, p);
        }
        void Merge(const AABB &other)
        {
                min = glm::min(min, other.min);
                max = glm::max(max, other.max);
        }

        bool Contains(const glm::vec3 &p) const
        {
                return glm::all(glm::greaterThanEqual(p, min)) && glm::all(glm::lessThanEqual(p, max));
        }
        bool Intersects(const AABB &other) const
        {
                return glm::all(glm::lessThanEqual(min, other.max)) && glm::all(glm::greaterThanEqual(max, other.min));
        }

        static AABB MakeInvalid() { return AABB{}; }
};

// ---------------------------------------------------------------------------
// VertexAttributeFlags
// Bitmask stored per-primitive. Set by buildModel(), read by the renderer
// to know what attributes are packed into vertexData and at what stride.
// Lives here so Model owns its own vertex layout description.
// ---------------------------------------------------------------------------

enum VertexAttributeFlags : uint32_t
{
        ATTRIB_NONE      = 0,
        ATTRIB_POSITION  = 1 << 0,  // vec3  -> always present
        ATTRIB_NORMAL    = 1 << 1,  // vec3
        ATTRIB_TANGENT   = 1 << 2,  // vec4  (xyz + handedness)
        ATTRIB_TEXCOORD0 = 1 << 3,  // vec2
        ATTRIB_TEXCOORD1 = 1 << 4,  // vec2
        ATTRIB_TEXCOORD2 = 1 << 5,  // vec2
        ATTRIB_COLOR     = 1 << 6,  // vec4
        ATTRIB_JOINTS    = 1 << 7,  // uvec4
        ATTRIB_WEIGHTS   = 1 << 8,  // vec4
};

// ---------------------------------------------------------------------------
// MeshPrimitive -> one draw call's worth of geometry
// ---------------------------------------------------------------------------

struct MeshPrimitive
{
        enum class Mode : uint8_t
        {
                Points        = 0,
                Lines         = 1,
                LineLoop      = 2,
                LineStrip     = 3,
                Triangles     = 4,
                TriangleStrip = 5,
                TriangleFan   = 6,
        } mode = Mode::Triangles;

        Index materialIndex = INVALID_INDEX;

        // Packed, interleaved vertex data.
        // Layout: [pos][normal?][tangent?][uv0?][uv1?][uv2?][color?][joints?][weights?]
        // Attribute presence and per-vertex byte stride are described below.
        // Freed by Model::freeCPU() after GPU upload.
        std::vector<uint8_t>  vertexData;
        std::vector<uint32_t> indices;
        uint32_t              vertexCount    = 0;
        uint32_t              vertexStride   = 0;  // bytes per vertex
        uint32_t              attributeFlags = ATTRIB_NONE;

        std::vector<float> morphWeights;

        AABB bounds;
};

// ---------------------------------------------------------------------------
// Mesh
// ---------------------------------------------------------------------------

struct Mesh
{
        std::string                name;
        std::vector<MeshPrimitive> primitives;
        AABB                       bounds;
};

// ---------------------------------------------------------------------------
// Node
// ---------------------------------------------------------------------------

struct Node
{
        std::string name;

        glm::vec3 translation    = glm::vec3(0.0f);
        glm::quat rotation       = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale          = glm::vec3(1.0f);
        glm::mat4 localTransform = glm::mat4(1.0f);
        glm::mat4 worldTransform = glm::mat4(1.0f);  // computed during hierarchy pass

        Index meshIndex   = INVALID_INDEX;
        Index skinIndex   = INVALID_INDEX;
        Index cameraIndex = INVALID_INDEX;
        Index lightIndex  = INVALID_INDEX;

        std::vector<Index> children;
        Index              parent = INVALID_INDEX;
};

// ---------------------------------------------------------------------------
// ModelCamera
// ---------------------------------------------------------------------------

struct ModelCamera
{
        enum class Type : uint8_t
        {
                Perspective,
                Orthographic
        } type = Type::Perspective;

        struct PerspectiveData
        {
                float aspectRatio = 0.0f;
                float yfov        = 0.0f;
                float zfar        = 0.0f;
                float znear       = 0.0f;
        };

        struct OrthographicData
        {
                float xmag  = 0.0f;
                float ymag  = 0.0f;
                float zfar  = 0.0f;
                float znear = 0.0f;
        };

        std::string      name;
        PerspectiveData  perspective;
        OrthographicData orthographic;
};

// ---------------------------------------------------------------------------
// Image -> decoded RGBA8 pixel data (CPU-side only)
// Freed by Model::freeCPU() after GPU upload.
// ---------------------------------------------------------------------------

struct Image
{
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;  // original channel count before RGBA8 conversion
        bool     srgb     = false;
        string   name;

        std::vector<uint8_t> pixels;  // always RGBA8: width * height * 4 bytes

#if defined(IC_ASSET_NAMES)
        std::string name;
#endif
};

// ---------------------------------------------------------------------------
// Sampler -> texture filter/wrap state
// Enum values match OpenGL constants for zero-cost API mapping.
// ---------------------------------------------------------------------------

struct Sampler
{
        enum class Filter : uint16_t
        {
                Nearest              = 9728,
                Linear               = 9729,
                NearestMipMapNearest = 9984,
                LinearMipMapNearest  = 9985,
                NearestMipMapLinear  = 9986,
                LinearMipMapLinear   = 9987,
                None                 = 0
        };

        enum class Wrap : uint16_t
        {
                ClampToEdge    = 33071,
                MirroredRepeat = 33648,
                Repeat         = 10497,
                None           = 0
        };

        Filter magFilter = Filter::None;
        Filter minFilter = Filter::None;
        Wrap   wrapS     = Wrap::None;
        Wrap   wrapT     = Wrap::None;
};

// ---------------------------------------------------------------------------
// Skin
// ---------------------------------------------------------------------------

struct Skin
{
        std::string            name;
        std::vector<Index>     jointIndices;
        std::vector<glm::mat4> inverseBindMatrices;
        Index                  skeletonRootIndex = INVALID_INDEX;
};

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

struct AnimationSampler
{
        enum class Interpolation : uint8_t
        {
                Linear,
                Step,
                CubicSpline
        } interpolation = Interpolation::Linear;

        std::vector<float>     inputTimes;
        std::vector<glm::vec4> outputValues;  // vec4 covers all target types
};

struct AnimationChannel
{
        enum class Path : uint8_t
        {
                Translation,
                Rotation,
                Scale,
                Weights
        } targetPath = Path::Translation;

        Index samplerIndex    = INVALID_INDEX;
        Index targetNodeIndex = INVALID_INDEX;
};

struct Animation
{
        std::string                   name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float                         duration = 0.0f;
};

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------

struct Scene
{
        std::string        name;
        std::vector<Index> rootNodes;
};

// ---------------------------------------------------------------------------
// Model -> the runtime asset
//
// Lifecycle states:
//   Unloaded  → load() called            → Pending
//   Pending   → CPU data ready           → CPUReady
//   CPUReady  → GPU upload complete      → GPUReady
//   GPUReady  → freeCPU() called         → GPUReady  (pixel/vertex data freed)
//   any       → release() called         → Unloaded
//   any       → unrecoverable error      → Failed
// ---------------------------------------------------------------------------

class Model : public IAsset
{
        // Only these two may write into private data.
        // A free function is preferred over a friend class -> it grants
        // access to exactly one operation rather than an entire class scope.
        friend Model ic::build_model(ic::ModelImportData *data, IC_GUID id);
        friend class ic::Serializer;

public:
        ASSET_CLASS_TYPE(ASSET_TYPE_MODEL)

        explicit Model(IC_GUID id) : IAsset(id) {}

        // Non-copyable -> owns potentially large vertex/pixel buffers
        Model(const Model &)            = delete;
        Model &operator=(const Model &) = delete;
        Model(Model &&)                 = default;
        Model &operator=(Model &&)      = default;

        /**
         * Load from disk using the IModelLoader.
         * Transitions: Unloaded → Pending → CPUReady (or Failed)
         */
        bool Load(const char *filepath) override;

        /**
         * Fast file load from a .icmodel binary.
         * Skips the loader and builder entirely.
         * Transitions: Unloaded → CPUReady
         */
        bool SerializedLoad(ic::Serializer *s) override;

        /** Write CPUReady data to .icmodel binary for future fast file loads. */
        bool SerializedSave(ic::Serializer *s) const override;

        /**
         * Release all CPU and GPU data. Returns to Unloaded.
         * Safe to call in any state.
         */
        bool Release() override;

        bool IsLoaded() const override { return m_state >= State::CPUReady; }
        /**
         * Free CPU-side vertexData and Image::pixels after successful GPU upload.
         * State stays GPUReady. After this call the model cannot be re-uploaded
         * without reloading from disk or cache.
         */
        void FreeCPU();

        enum class State : uint8_t
        {
                Unloaded = 0,
                Pending,   // load() in progress
                CPUReady,  // data in RAM, GPU upload pending
                GPUReady,  // data on GPU (CPU data may or may not still be present)
                Failed
        };

        State getState() const { return m_state; }
        bool  isCPUReady() const { return m_state == State::CPUReady || m_state == State::GPUReady; }
        bool  isGPUReady() const { return m_state == State::GPUReady; }

        // -----------------------------------------------------------------------
        // Accessors -> all const, no copies
        // -----------------------------------------------------------------------

        const std::vector<Mesh>        &meshes() const { return m_meshes; }
        const std::vector<Material>    &materials() const { return m_materials; }
        const std::vector<Image>       &images() const { return m_images; }
        const std::vector<Sampler>     &samplers() const { return m_samplers; }
        const std::vector<Node>        &nodes() const { return m_nodes; }
        const std::vector<ModelCamera> &cameras() const { return m_cameras; }
        const std::vector<Skin>        &skins() const { return m_skins; }
        const std::vector<Animation>   &animations() const { return m_animations; }
        const std::vector<Scene>       &scenes() const { return m_scenes; }

        const AABB  &GetWorldBounds() const { return m_worldBounds; }
        Index        GetDefaultSceneIndex() const { return m_defaultScene; }
        const Scene *GetDefaultScene() const { return GetScene(m_defaultScene); }

        size_t GetMeshCount() const { return m_meshes.size(); }
        size_t GetNodeCount() const { return m_nodes.size(); }
        size_t GetMaterialCount() const { return m_materials.size(); }
        size_t GetAnimationCount() const { return m_animations.size(); }

        const Mesh      *GetMesh(Index i) const { return i < m_meshes.size() ? &m_meshes[i] : nullptr; }
        const Node      *GetNode(Index i) const { return i < m_nodes.size() ? &m_nodes[i] : nullptr; }
        const Material  *GetMaterial(Index i) const { return i < m_materials.size() ? &m_materials[i] : nullptr; }
        const Animation *GetAnimation(Index i) const { return i < m_animations.size() ? &m_animations[i] : nullptr; }
        const Scene     *GetScene(Index i) const { return i < m_scenes.size() ? &m_scenes[i] : nullptr; }
        const Image     *GetImage(Index i) const { return i < m_images.size() ? &m_images[i] : nullptr; }
        const Sampler   *GetSampler(Index i) const { return i < m_samplers.size() ? &m_samplers[i] : nullptr; }

private:
        std::vector<Mesh>        m_meshes;
        std::vector<Material>    m_materials;
        std::vector<Image>       m_images;
        std::vector<Sampler>     m_samplers;
        std::vector<Node>        m_nodes;
        std::vector<ModelCamera> m_cameras;
        std::vector<Skin>        m_skins;
        std::vector<Animation>   m_animations;
        std::vector<Scene>       m_scenes;

        AABB  m_worldBounds;
        Index m_defaultScene = INVALID_INDEX;
        State m_state        = State::Unloaded;
};

}  // namespace ic

#endif
