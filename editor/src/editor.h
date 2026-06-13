#ifndef EDITOR_H
#define EDITOR_H

#include <ic_engine.h>
#include <renderer/opengl/gl_framebuffer.h>  // we are going to use gl_framebuffer for now.

namespace ic
{

struct EditorState
{
        Entity selected;

        Entity renameTarget;
        char   renameBuffer[256];
        bool   renamingScene;
        bool   isFocused;

        // Each scene has or model has some meshes each have some submeshes (the actual meshes that contain the material
        // and data)
        int32_t selectedSubmesh;
        int32_t selectedMesh;

        float lastCameraPanelHeight = 0.0f;
};

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
        EditorState m_State;
        void        DrawSceneHeirarchy();

        // Heirarchy helperss
        void DrawSceneNode();                      // scene root tree node + rename + context
        void DrawSceneContextMenu();               // create entity / lighting submenu
        void DrawEntityNode(ic::Entity e);         // one entity row, dispatches rename vs normal
        void DrawEntityRename(ic::Entity e);       // inline rename for an entity
        void DrawEntityContextMenu(ic::Entity e);  // rename / duplicate / delete
        void DrawMeshNodes(ic::Entity e);          // selectable mesh child rows
        void DrawSubmeshNodes(ic::Entity e, Mesh &mesh, int selectedMesh);  // submesh object drawing logic
        void DrawCameraPanel();                                             // bottom panel + height tracking
        bool DrawRenameField();                                             // shared focus+input+commit widget

        // Material Editor
        void DrawMaterialEditor(Entity &e);  // Draws material editor for a submesh of an entity
        bool EditMaterial(Material *mat);    // Edits the material,
        void ChangeTexture(Texture &e,
                           Mesh    &mesh);  // Changes the texture asset for the given submesh for the given entity

        // Editor Resources
        string LoadLastScenePath();
        void   ClearColor();
        void   SaveLastScenePath(const char *path);
};

}  // namespace ic

#endif  // EDITOR_H