#include "renderer/opengl/gl_framebuffer.h"

#include <glad/glad.h>
#include <utility>
#include <array>

namespace ic {

namespace {

// Map our format enum to a GL internal format
GLenum ToGLInternalFormat(FramebufferTextureFormat fmt) {
	switch (fmt) {
		case FramebufferTextureFormat::RGBA8:              return GL_RGBA8;
		case FramebufferTextureFormat::RGBA16F:            return GL_RGBA16F;
		case FramebufferTextureFormat::RED_INTEGER:        return GL_R32I;
		case FramebufferTextureFormat::DEPTH24_STENCIL8:   return GL_DEPTH24_STENCIL8;
		case FramebufferTextureFormat::DEPTH32F:           return GL_DEPTH_COMPONENT32F;
		default:
			IC_CORE_ASSERT(false, "Framebuffer: unknown texture format");
			return 0;
	}
}

// Data format (used in glTexImage2D / glReadPixels)
GLenum ToGLDataFormat(FramebufferTextureFormat fmt) {
    	switch (fmt) {
        	case FramebufferTextureFormat::RGBA8:
        	case FramebufferTextureFormat::RGBA16F:     return GL_RGBA;
        	case FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
        default:
            	IC_CORE_ASSERT(false, "Framebuffer: no data format for depth");
            	return 0;
    	}
}

// Data type for glTexImage2D / glReadPixels
GLenum ToGLDataType(FramebufferTextureFormat fmt) {
	switch (fmt) {
		case FramebufferTextureFormat::RGBA8:       return GL_UNSIGNED_BYTE;
		case FramebufferTextureFormat::RGBA16F:     return GL_FLOAT;
		case FramebufferTextureFormat::RED_INTEGER: return GL_INT;
	default:
		IC_CORE_ASSERT(false, "Framebuffer: no data type for depth");
		return 0;
	}
}

// Attach a color texture to the FBO at the given color index
void AttachColorTexture(uint32_t texID, uint32_t colorIndex,
                        GLenum internalFmt, GLenum dataFmt, GLenum dataType,
                        uint32_t width, uint32_t height, bool linear) 
{
	glBindTexture(GL_TEXTURE_2D, texID);
    	glTexImage2D(GL_TEXTURE_2D, 0, internalFmt,
		static_cast<GLsizei>(width),
		static_cast<GLsizei>(height),
		0, dataFmt, dataType, nullptr);

    	GLenum filter = linear ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	// Clamp so sampling outside [0,1] doesn't wrap — important for picking
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0 + colorIndex,
                           GL_TEXTURE_2D, texID, 0);
}

// Attach a depth (or depth-stencil) texture to the FBO
void AttachDepthTexture(uint32_t texID, GLenum internalFmt,
                        GLenum attachment,
                        uint32_t width, uint32_t height) 
{
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFmt,
			static_cast<GLsizei>(width),
			static_cast<GLsizei>(height),
			0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

	// Depth textures don't need filtering for our use cases
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texID, 0);
}

} // anonymous namespace

// -------------------------------------------------------
// Static helpers
// -------------------------------------------------------
bool Framebuffer::IsDepthFormat(FramebufferTextureFormat fmt) {
    	return fmt == FramebufferTextureFormat::DEPTH24_STENCIL8 ||
        	   fmt == FramebufferTextureFormat::DEPTH32F;
}

bool Framebuffer::IsColorFormat(FramebufferTextureFormat fmt) {
    	return !IsDepthFormat(fmt) && fmt != FramebufferTextureFormat::None;
}

// -------------------------------------------------------
// Constructor / Destructor
// -------------------------------------------------------
Framebuffer::Framebuffer(FramebufferSpec& spec) : m_spec(spec) 
{
	for (auto& att : m_spec.attachments) {
		if (IsDepthFormat(att.format))
			m_depthSpec = att;
		else
			m_colorSpecs.push_back(att);
	}

	IC_CORE_ASSERT(!m_colorSpecs.empty() || m_depthSpec.format != FramebufferTextureFormat::None,
			"Framebuffer: no attachments specified");

	Invalidate();
}

Framebuffer::~Framebuffer() {
	Release();
}

// -------------------------------------------------------
// Move semantics
// -------------------------------------------------------
Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_spec(std::move(other.m_spec))
    , m_fbo(other.m_fbo)
    , m_colorAttachments(std::move(other.m_colorAttachments))
    , m_depthAttachment(other.m_depthAttachment)
    , m_colorSpecs(std::move(other.m_colorSpecs))
    , m_depthSpec(other.m_depthSpec)
{
	other.m_fbo            = 0;
	other.m_depthAttachment= 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        Release();
        m_spec             = std::move(other.m_spec);
        m_fbo              = other.m_fbo;
        m_colorAttachments = std::move(other.m_colorAttachments);
        m_depthAttachment  = other.m_depthAttachment;
        m_colorSpecs       = std::move(other.m_colorSpecs);
        m_depthSpec        = other.m_depthSpec;
        other.m_fbo            = 0;
        other.m_depthAttachment= 0;
    }
    return *this;
}

// -------------------------------------------------------
// Invalidate — (re)create all GL objects
// -------------------------------------------------------
void Framebuffer::Invalidate() {
    	// Destroy existing objects if recreating
	if (m_fbo) {
		Release();
		// Re-split specs in case they were cleared by Release
		m_colorSpecs.clear();
		m_depthSpec = {};
		for (auto& att : m_spec.attachments) {
			if (IsDepthFormat(att.format)) m_depthSpec = att;
			else                           
				m_colorSpecs.push_back(att);
		}
	}

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	// ---- Color attachments ----
	m_colorAttachments.resize(m_colorSpecs.size(), 0);

	if (!m_colorSpecs.empty()) {
		glGenTextures(static_cast<GLsizei>(m_colorSpecs.size()),
			m_colorAttachments.data());

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_colorSpecs.size()); i++) {
		auto& spec = m_colorSpecs[i];
		AttachColorTexture(
			m_colorAttachments[i],
			i,
			ToGLInternalFormat(spec.format),
			ToGLDataFormat(spec.format),
			ToGLDataType(spec.format),
			m_spec.width,
			m_spec.height,
			spec.linearFilter
		);
		}
	}

	// ---- Depth attachment ----
	if (m_depthSpec.format != FramebufferTextureFormat::None) {
		glGenTextures(1, &m_depthAttachment);

		GLenum glInternal  = ToGLInternalFormat(m_depthSpec.format);
		GLenum glAttachment = (m_depthSpec.format == FramebufferTextureFormat::DEPTH24_STENCIL8)
				? GL_DEPTH_STENCIL_ATTACHMENT
				: GL_DEPTH_ATTACHMENT;

		AttachDepthTexture(m_depthAttachment, glInternal, glAttachment,
				m_spec.width, m_spec.height);
	}

    // ---- Tell GL which color attachments to draw into ----
	if (m_colorAttachments.size() > 1) {
		IC_CORE_ASSERT(m_colorAttachments.size() <= 4,
			"Framebuffer: more than 4 color attachments not supported");

		std::array<GLenum, 4> drawBuffers = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		};
		glDrawBuffers(static_cast<GLsizei>(m_colorAttachments.size()),
			drawBuffers.data());
	} else if (m_colorAttachments.empty()) {
		// Depth-only FBO (shadow maps)
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	m_hasIntegerAttachment = false;
	for (auto& spec : m_colorSpecs) {
		if (spec.format == FramebufferTextureFormat::RED_INTEGER) 
		{
			m_hasIntegerAttachment = true;
			break;
		}
	}

    // ---- Completeness check ----
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		IC_CORE_WARN("Framebuffer incomplete: status = 0x{:X}", status);
	} else {
		IC_CORE_INFO("Framebuffer created: {}x{}, {} color attachment(s)",
			m_spec.width, m_spec.height, m_colorAttachments.size());
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// -------------------------------------------------------
// Release — destroy all GL objects
// -------------------------------------------------------
void Framebuffer::Release() {
	if (m_fbo) {
		glDeleteFramebuffers(1, &m_fbo);
		m_fbo = 0;
	}
	if (!m_colorAttachments.empty()) {
		glDeleteTextures(static_cast<GLsizei>(m_colorAttachments.size()),
				m_colorAttachments.data());
		m_colorAttachments.clear();
	}
	if (m_depthAttachment) {
		glDeleteTextures(1, &m_depthAttachment);
		m_depthAttachment = 0;
	}
}

// -------------------------------------------------------
// Bind / Unbind
// -------------------------------------------------------

// TODO: fix whatever this is 
void Framebuffer::Bind() const {
	IC_CORE_ASSERT(m_fbo, "Framebuffer::Bind called on invalid FBO");
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0,
		static_cast<GLsizei>(m_spec.width),
		static_cast<GLsizei>(m_spec.height));

	if (m_hasIntegerAttachment) {
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
    	}
}

void Framebuffer::Unbind() const {
    	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Restore blending for the default framebuffer
	// (ImGui and your scene color pass need it)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DITHER);
}

// -------------------------------------------------------
// Resize
// -------------------------------------------------------
void Framebuffer::Resize(uint32_t width, uint32_t height) {
	// Guard against degenerate sizes (minimized window etc)
	if (width == 0 || height == 0 || width > 8192 || height > 8192) {
		IC_CORE_WARN("Framebuffer::Resize ignored: {}x{}", width, height);
		return;
	}
	if (width == m_spec.width && height == m_spec.height)
		return; // nothing to do

	m_spec.width  = width;
	m_spec.height = height;
	Invalidate();
}

// -------------------------------------------------------
// Clear
// -------------------------------------------------------
void Framebuffer::ClearAll() const {
	Bind();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	Unbind();
}

void Framebuffer::ClearColorAttachment(uint32_t attachmentIndex, int clearValue) const {
	IC_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(),
			"Framebuffer::ClearColorAttachment: index out of range");

	// Only RED_INTEGER attachments make sense to clear with an int
	auto& spec = m_colorSpecs[attachmentIndex];
	IC_CORE_ASSERT(spec.format == FramebufferTextureFormat::RED_INTEGER,
			"Framebuffer::ClearColorAttachment: use ClearAll for non-integer attachments");

	glClearTexImage(m_colorAttachments[attachmentIndex],
			0,
			ToGLDataFormat(spec.format),
			ToGLDataType(spec.format),
			&clearValue);
}

// -------------------------------------------------------
// Accessors
// -------------------------------------------------------
uint32_t Framebuffer::GetColorAttachment(uint32_t index) const {
	IC_CORE_ASSERT(index < m_colorAttachments.size(),
			"Framebuffer::GetColorAttachment: index out of range");
	return m_colorAttachments[index];
}

uint32_t Framebuffer::GetDepthAttachment() const {
    	return m_depthAttachment;
}

// -------------------------------------------------------
// Pixel read-back (entity picking)
// -------------------------------------------------------
int Framebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y) const {
    	IC_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(),
                   "Framebuffer::ReadPixel: attachment index out of range");

	Bind();
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

	// OpenGL origin is bottom-left; ImGui/window coords are top-left — flip y
	int flippedY = static_cast<int>(m_spec.height) - y - 1;

	int pixelValue = -1;
	glReadPixels(x, flippedY, 1, 1,
			ToGLDataFormat(m_colorSpecs[attachmentIndex].format),
			ToGLDataType(m_colorSpecs[attachmentIndex].format),
			&pixelValue);

	Unbind();
	return pixelValue;
}

} // namespace ic