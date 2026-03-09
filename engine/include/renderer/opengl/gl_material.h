#ifndef GL_MATERIAL_H
#define GL_MATERIAL_H

#include "defines.h"
#include "core/assets/types/model.h"
#include "core/assets/types/material.h"

#include <glad/glad.h>

/**
 * gl_material.h -> GPU-side texture management.
 *
 * GLTexture is the only type here. Material state (uniforms, blend mode, cull face) is applied
 * per draw call in GLModel::bindMaterial(), which has direct access to
 * the shader and GL state machine. A GLMaterial wrapper would just be
 * an indirection around setUniform calls with no GPU object to own.
 *
 * later move to a bindless or UBO-based material system, a
 * GLMaterial owning a UBO handle would make sense. Not now.
 *
 * Ownership:
 *   GLTexture owns one GL texture object (textureHandle).
 *   Call destroy() before discarding or reassigning.
 *   GLModel::m_textures[] is the authoritative owner -> indexed by
 *   Model::images() index, not the GLTF texture-list index.
 */

namespace ic
{

struct GLTexture
{
        GLuint textureHandle = 0;

        /**
         * Allocate and upload a GL_TEXTURE_2D from a runtime Image.
         * Image must be RGBA8 (always true after buildModel).
         * Generates mipmaps. Applies default linear filtering.
         * Safe to call on an already-uploaded texture -> destroys the
         * previous handle first.
         */
        void upload(const Image &img);

        /**
         * Apply sampler filter and wrap state to this texture.
         * Call after upload(). Can be called again if the sampler changes.
         * No-op if textureHandle == 0.
         */
        void applySampler(const Sampler *sampler);

        /**
         * Delete the GL texture object. Sets textureHandle to 0.
         * Safe to call if upload() was never called or destroy() was
         * already called.
         */
        void destroy();

        bool isValid() const { return textureHandle != 0; }
};

}  // namespace ic

#endif  // GL_MATERIAL_H