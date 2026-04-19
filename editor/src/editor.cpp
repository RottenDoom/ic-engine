#include "editor.h"

#include <renderer/scene_serializer.h>
#include "panels/panels.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace ic
{

void EditorSystem::Init()
{
        m_EditorConfig = "assets/config.yaml";
        m_DefaultScene = "assets/default.scene";

        m_EditorCamera = createCamera(Camera::CameraType::firstperson, glm::vec3(0.0f, 0.0f, -5.0f));

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // enable docking and nav keyboard
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;      // Enable Multi-Viewport // Exception on
        // platform windows? FIx?
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        // some boilerplate code
        // TODO: this style must be changeable by in editor layout or some other lib
        ImGui::StyleColorsDark();
        ImGuiStyle &style       = ImGui::GetStyle();
        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 3.0f;
        style.PopupRounding     = 4.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding      = 3.0f;
        style.TabRounding       = 4.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;

        // Slightly warmer dark palette
        auto *colors                      = style.Colors;
        colors[ImGuiCol_WindowBg]         = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_Header]           = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered]    = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderActive]     = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
        colors[ImGuiCol_FrameBg]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_Button]           = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_ButtonHovered]    = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
        colors[ImGuiCol_ButtonActive]     = ImVec4(0.40f, 0.40f, 0.44f, 1.00f);
        colors[ImGuiCol_TitleBg]          = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive]    = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_Tab]              = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_TabHovered]       = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
        colors[ImGuiCol_TabActive]        = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_CheckMark]        = ImVec4(0.56f, 0.83f, 0.26f, 1.00f);
        colors[ImGuiCol_SliderGrab]       = ImVec4(0.56f, 0.83f, 0.26f, 0.80f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.83f, 0.26f, 1.00f);

        Application &app    = Application::Get();
        GLFWwindow  *window = app.GetWindow()->GetNativeWindow();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 450");

        FramebufferSpec fbSpec;
        fbSpec.attachments = {FramebufferTextureFormat::RGBA8,
                              // FramebufferTextureFormat::RED_INTEGER,
                              FramebufferTextureFormat::DEPTH24_STENCIL8};
        fbSpec.width       = 1280;
        fbSpec.height      = 720;

        m_fb = new Framebuffer(fbSpec);

        /** TODO: Load default scene if last scene does not exist. */
        ic::SceneSerializer serializer;
        string              lastScene = LoadLastScenePath();
        if (!lastScene.empty() && ic_exists(lastScene.c_str()))
        {
                m_ActiveScene = serializer.Deserialize(lastScene.c_str());
                m_ScenePath   = lastScene.c_str();
                IC_INFO("Editor: Loaded scene {}", lastScene);
        }
        else
        {
                m_ActiveScene = new ic::RenderScene("New Scene");
                IC_CORE_INFO("Editor: Started with empty scene");
        }

        ic_set_scene(m_ActiveScene, m_EditorCamera);

        // panel commands register
}

void EditorSystem::Update(float dt)
{
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // m_ViewportHovered is set in Render() each frame — one frame behind, which is fine (~16ms at 60fps).
        bool allowMouseInput        = m_ViewportHovered;
        m_EditorCamera.inputEnabled = allowMouseInput;

        if (allowMouseInput)
        {
                m_EditorCamera.OnUpdate(dt);
        }

        if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
                m_fb->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        m_fb->Bind();
        ic_clear_buffer_bit();

        // Resize
        if (FramebufferSpec spec = m_fb->GetSpec(); m_ViewportSize.x > 0.0f &&
                                                    m_ViewportSize.y > 0.0f &&  // zero sized framebuffer is invalid
                                                    (spec.width != m_ViewportSize.x || spec.height != m_ViewportSize.y))
        {
                m_fb->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
                // m_EditorCamera.onResize(m_ViewportSize.x, m_ViewportSize.y);
                m_EditorCamera.SetViewPortSize(m_ViewportSize.x, m_ViewportSize.y);
        }
}

void EditorSystem::Render()
{
        m_fb->Unbind();

        // dockspace — must be submitted before any window that docks into it
        {
                ImGuiViewport   *vp        = ImGui::GetMainViewport();
                ImGuiWindowFlags dockFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                ImGui::SetNextWindowPos(vp->Pos);
                ImGui::SetNextWindowSize(vp->Size);
                ImGui::SetNextWindowViewport(vp->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::Begin("##DockspaceRoot", nullptr, dockFlags);

                // --- Main menu bar ---
                {
                        static char s_OpenPath[512]   = "assets/scenes/";
                        static char s_SaveAsPath[512] = "assets/scenes/";
                        bool        doOpenPopup = false, doSaveAsPopup = false;

                        if (ImGui::BeginMainMenuBar())
                        {
                                if (ImGui::BeginMenu("File"))
                                {
                                        if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                                                NewScene();
                                        if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                                                doOpenPopup = true;
                                        ImGui::Separator();
                                        if (ImGui::MenuItem("Save", "Ctrl+S"))
                                                SaveSceneAs(m_ScenePath);
                                        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                                                doSaveAsPopup = true;
                                        ImGui::EndMenu();
                                }
                                ImGui::EndMainMenuBar();
                        }

                        // Defer OpenPopup calls until after EndMainMenuBar
                        if (doOpenPopup)
                                ImGui::OpenPopup("Open Scene");
                        if (doSaveAsPopup)
                        {
                                if (!m_ScenePath.empty())
                                        snprintf(s_SaveAsPath, sizeof(s_SaveAsPath), "%s", m_ScenePath);
                                ImGui::OpenPopup("Save Scene As");
                        }

                        ImVec2 center = ImGui::GetMainViewport()->GetCenter();

                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                        {
                                ImGui::Text("Scene path:");
                                ImGui::SetNextItemWidth(400.0f);
                                ImGui::InputText("##openpath", s_OpenPath, sizeof(s_OpenPath));
                                if (ImGui::Button("Open", ImVec2(120, 0)))
                                {
                                        OpenScene(string(s_OpenPath));
                                        ImGui::CloseCurrentPopup();
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                                        ImGui::CloseCurrentPopup();
                                ImGui::EndPopup();
                        }

                        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                        if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                        {
                                ImGui::Text("Save path:");
                                ImGui::SetNextItemWidth(400.0f);
                                ImGui::InputText("##savepath", s_SaveAsPath, sizeof(s_SaveAsPath));
                                if (ImGui::Button("Save", ImVec2(120, 0)))
                                {
                                        SaveSceneAs((string &)s_SaveAsPath);
                                        ImGui::CloseCurrentPopup();
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                                        ImGui::CloseCurrentPopup();
                                ImGui::EndPopup();
                        }
                }

                ImGui::PopStyleVar(3);
                ImGui::DockSpace(ImGui::GetID("MainDockspace"), ImVec2(0, 0), 0);
                ImGui::End();
        }

        // viewport — scene FBO as texture
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);  // first use size
        ImGui::Begin("Viewport");
        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();
        ImVec2 size       = ImGui::GetContentRegionAvail();
        if (size.x < 1.0f)
                size.x = 1.0f;
        if (size.y < 1.0f)
                size.y = 1.0f;
        m_ViewportSize = {size.x, size.y};

        // Resize BEFORE drawing so image matches FBO this frame
        if ((uint32_t)size.x != m_fb->GetWidth() || (uint32_t)size.y != m_fb->GetHeight())
                m_fb->Resize((uint32_t)size.x, (uint32_t)size.y);

        ImGui::Image((ImTextureID)(uintptr_t)m_fb->GetColorAttachment(0), size, ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::ShowDemoWindow();
        ic::panels::heirarchy_draw(m_ActiveScene, m_State);
        ic::panels::component_panel_draw(m_State.selected);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorSystem::Shutdown()
{
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_fb)
        {
                m_fb->Unbind();
        }

        delete m_fb;
        delete m_ActiveScene;

        m_fb          = nullptr;
        m_ActiveScene = nullptr;
}

void EditorSystem::NewScene()
{
        delete m_ActiveScene;
        m_ActiveScene = new ic::RenderScene("New Scene");

        // new scene makes an untitled scene
        char buffer[128];
        m_SavedSceneCount++;
        snprintf(buffer, sizeof(buffer), "assets/scenes/untitled_%d.scene", m_SavedSceneCount);
        m_ScenePath = buffer;

        m_State.selected = {};
        ic_set_scene(m_ActiveScene, m_EditorCamera);
        IC_INFO("Editor: new scene created");
}

void EditorSystem::SaveScene()
{
        /** ISSUE: If ctrl s is pressed an option to save scene must also exist */
        ic::SceneSerializer serializer;
        serializer.Serialize(m_ScenePath.c_str(), m_ActiveScene);
        SaveLastScenePath(m_ScenePath.c_str());
        IC_INFO("Editor: saved scene to {}", m_ScenePath);
}

void EditorSystem::SaveSceneAs(string &path)
{
        char *parent = fs_getParentPath(path.c_str());
        IC_CORE_ASSERT(parent, "Parent path does not exist!");
        if (!ic_exists(parent))
        {
                IC_INFO("Creating directory: {}", parent);
                ic_mkdir(parent);
        }

        m_ScenePath = path;
        SaveScene();
}

void EditorSystem::OpenScene(const string &path)
{
        if (!ic_exists(path.c_str()))
        {
                IC_CORE_WARN("Editor: scene not found: {}", path);
                return;
        }
        ic::SceneSerializer serializer;
        delete m_ActiveScene;
        m_ActiveScene    = serializer.Deserialize(path.c_str());
        m_ScenePath      = path;
        m_State.selected = {};
        SaveLastScenePath(path.c_str());
        ic_set_scene(m_ActiveScene, m_EditorCamera);
        IC_CORE_INFO("Editor: opened scene {}", path);
}

// These functions are used with multiple scenes and editor config with a scene for now we are working with default
// scne.
string EditorSystem::LoadLastScenePath()
{
        if (!ic_exists(m_EditorConfig))
        {
                IC_CORE_CRITICAL("Editor config does not exist cannot open editor!");
                return "";
        }
        try
        {
                // use my own filesystem here.
                std::ifstream f(m_EditorConfig);
                string        line;
                while (getline(f, line))
                {
                        auto pos = line.find("last_scene:");
                        if (pos != string::npos)
                        {
                                string val = line.substr(pos + 11);
                                // trim whitespace and quotes
                                auto s = val.find_first_not_of(" \t\"");
                                auto e = val.find_last_not_of(" \t\"");
                                if (s != std::string::npos)
                                        return val.substr(s, e - s + 1);
                        }
                }
        }
        catch (const std::exception &e)
        {
                IC_CORE_WARN("EditorSystem: failed to read config: {}", e.what());
                return "";
        }

        // return default scene if last scene does not exist
        return m_DefaultScene;
}

void EditorSystem::SaveLastScenePath(const char *path)
{
        const char *parent = fs_getParentPath(path);
        if (!fs_mkdir(parent))
        {
                /** FIX: THIS part needs fixiing. */
                IC_CORE_WARN("EditorSystem: cannot create parent path");
        }
        std::ofstream f(m_EditorConfig);
        f << "last_scene: \"" << path << "\"\n";
}

}  // namespace ic
