#include <ic_engine.h>
#include <renderer/opengl/gl_framebuffer.h>  // we are going to use gl_framebuffer for now.

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
        void SaveSceneAs(string &path);
        void OpenScene(const string &path);

private:
        Framebuffer *m_fb = nullptr;
        Camera       m_EditorCamera;

        // Refs to the scene
        const char      *m_EditorConfig;
        string           m_ScenePath;
        const char      *m_DefaultScene;
        ic::RenderScene *m_ActiveScene;
        ic::RenderScene *m_EditorScene;

        bool      m_ViewportFocused = false, m_ViewportHovered = false;
        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2];
        uint16_t  m_SavedSceneCount = 0;

        enum class SceneState
        {
                Edit = 0,
                Play,
                Simulate,
                Loading
        };

        // Panels
        ic::Entity m_SelectedEntity;

        // Editor Resources

        string LoadLastScenePath();
        void   ClearColor();
        void   SaveLastScenePath(const char *path);
};

}  // namespace ic
