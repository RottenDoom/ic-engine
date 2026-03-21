#include <ic_engine.h>
#include <renderer/opengl/gl_framebuffer.h> // we are going to use gl_framebuffer for now.

namespace ic
{

class EditorSystem 
{
public:
	void Init();
	void Update(float dt);
	void Render();
	void Shutdown();

	void NewScene();
	void SaveScene();
	void SaveSceneAs(const char* path);
	void OpenScene(const char* path);
private:
	// TODO : For now we need a framebuffer and some entities and a editor and active scene 
	// We also need some kind of serializatin for saving and opening scenes. 
	// Also need some panels and some kind viewport values and resize things which doesnt really depends on the application windows. 

	
	Framebuffer *m_fb = nullptr;
	Camera m_EditorCamera;

	// Refs to the scene
	const char* m_ScenePath = "assets/default_scene/scene.yaml";
	ic::RenderScene* m_ActiveScene;
	ic::RenderScene* m_EditorScene;

	bool m_ViewportFocused = false, m_ViewportHovered = false;
	glm::vec2 m_ViewportSize = {0.0f, 0.0f};
	glm::vec2 m_ViewportBounds[2];

	enum class SceneState {
		Edit = 0, Play, Simulate , Loading
	};

	// Panels

	// Editor Resources

	const char* LoadLastScenePath();
	void SaveLastScenePath(const char* path);
};

} // namespace ic::editor
