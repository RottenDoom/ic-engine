#ifndef GL_FRAMBUFFER_H
#define GL_FRAMBUFFER_H

#include "defines.h"

namespace ic
{

enum class FramebufferTextureFormat {
	None = 0,
	RGBA8,
	RGBA16F,
	RED_INTEGER,
	DEPTH24_STENCIL8,
	DEPTH32F,
};

struct FramebufferTextureSpec {
	FramebufferTextureFormat format = FramebufferTextureFormat::None;
	bool linearFilter = true;

	FramebufferTextureSpec() = default;
	FramebufferTextureSpec(FramebufferTextureFormat fmt) : format(fmt) {}
};

struct FramebufferSpec {
	uint32_t width = 1280;
	uint32_t height = 720;
	uint32_t samples = 1;

	std::vector<FramebufferTextureSpec> attachments;
};

class IC_API Framebuffer {
public:
	explicit Framebuffer(uint32_t width, uint32_t height, uint32_t samples, uint32_t flags);
   	~Framebuffer();

	Framebuffer(const Framebuffer&)            = delete;
    	Framebuffer& operator=(const Framebuffer&) = delete;

	Framebuffer(Framebuffer&& other) noexcept;
	Framebuffer& operator=(Framebuffer&& other) noexcept;

	void Bind() const;
	void Unbind() const;

	void Resize(uint32_t width, uint32_t height);

	void ClearAll() const;

	void ClearColorAttachment(uint32_t attachmentIdx, int clearValue) const;

	uint32_t GetColorAttachment(uint32_t index = 0) const;

	uint32_t GetDepthAttachment() const;

	int ReadPixel(uint32_t attachmentIndex, int x, int y) const;

	    uint32_t              GetWidth()  const { return m_spec.width;  }
	uint32_t              GetHeight() const { return m_spec.height; }
	const FramebufferSpec& GetSpec()  const { return m_spec;        }
	uint32_t              GetFBO()    const { return m_fbo;         }
	
	bool IsValid() const { return m_fbo != 0; }

private:
	void Invalidate();
	void Release();

	static bool IsDepthFormat(FramebufferTextureFormat fmt);
	static bool IsColorFormat(FramebufferTextureFormat fmt);
 
	FramebufferSpec          m_spec;

	uint32_t                 m_fbo            = 0;

	std::vector<uint32_t>    m_colorAttachments;  // GL texture IDs, one per color spec
	uint32_t                 m_depthAttachment  = 0;

	std::vector<FramebufferTextureSpec> m_colorSpecs;
	FramebufferTextureSpec              m_depthSpec;
};


} // namespace ic


#endif
