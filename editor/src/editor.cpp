#include "editor.h"

#include <renderer/scene_serializer.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace ic
{

void EditorSystem::Init()	{
	if (ImGui::GetCurrentContext() == nullptr) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();


		// enable docking and nav keyboard
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;      // Enable Multi-Viewport // Exception on platform windows? FIx?
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		// some boilerplate code
		// TODO: this style must be changeable by in editor layout or some other lib
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding   = 4.0f;
		style.FrameRounding    = 3.0f;
		style.PopupRounding    = 4.0f;
		style.ScrollbarRounding= 3.0f;
		style.GrabRounding     = 3.0f;
		style.TabRounding      = 4.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize  = 0.0f;

		// Slightly warmer dark palette
		auto* colors = style.Colors;
		colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
		colors[ImGuiCol_Header]          = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
		colors[ImGuiCol_HeaderHovered]   = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
		colors[ImGuiCol_HeaderActive]    = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
		colors[ImGuiCol_FrameBg]         = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
		colors[ImGuiCol_Button]          = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
		colors[ImGuiCol_ButtonHovered]   = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
		colors[ImGuiCol_ButtonActive]    = ImVec4(0.40f, 0.40f, 0.44f, 1.00f);
		colors[ImGuiCol_TitleBg]         = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
		colors[ImGuiCol_TitleBgActive]   = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
		colors[ImGuiCol_Tab]             = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
		colors[ImGuiCol_TabHovered]      = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
		colors[ImGuiCol_TabActive]       = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_CheckMark]       = ImVec4(0.56f, 0.83f, 0.26f, 1.00f);
		colors[ImGuiCol_SliderGrab]      = ImVec4(0.56f, 0.83f, 0.26f, 0.80f);
		colors[ImGuiCol_SliderGrabActive]= ImVec4(0.56f, 0.83f, 0.26f, 1.00f);

		Application& app = Application::Get();
		GLFWwindow* window = app.GetWindow()->GetNativeWindow();

		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init("#version 450");
	}
	// FramebufferSpec fbSpec = {};
	// Convert framebuffer attachments into bit flags.
	FramebufferSpec fbSpec;
	fbSpec.attachments = 
		{ 
			FramebufferTextureFormat::RGBA8, 
			// FramebufferTextureFormat::RED_INTEGER, 
			FramebufferTextureFormat::DEPTH24_STENCIL8 
		};
	fbSpec.width = 1280;
	fbSpec.height = 720;

	m_fb = new Framebuffer(fbSpec);

	ic::SceneSerializer serializer;
	const char* lastScene = LoadLastScenePath();
	if (lastScene && ic_exists(lastScene)) {
		m_ActiveScene = serializer.Deserialize(lastScene);
		m_ScenePath = lastScene;
		IC_INFO("Editor: Loaded scene {}", lastScene);
	} else {
		m_ActiveScene = new ic::RenderScene();
		IC_CORE_INFO("Editor: Started with bland scene");
	}

	ic_set_scene(m_ActiveScene);

	// panel commands register
}

void EditorSystem::Update(float dt) {
	// Input to UI system wont pass to camera different camera states

	// Resize
	// if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
	// 	m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
	// 	(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
	// {
	// 	m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
	// 	m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
	// 	m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
	// }

	// wantcapturesmouse too
	// panel::CommandPanelhandleInput
}

void EditorSystem::Render() {
	m_fb->Bind();

	// --- ImGui frame ---
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		ImGuiViewport* vp = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(vp->Pos);
		ImGui::SetNextWindowSize(vp->Size);
		ImGui::SetNextWindowViewport(vp->ID);

		ImGuiWindowFlags dockFlags =
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("##DockspaceRoot", nullptr, dockFlags);
		ImGui::PopStyleVar(3);

		// panels::MenuBarDraw();

		ImGuiID dockId = ImGui::GetID("MainDockspace");
		ImGui::DockSpace(dockId, ImVec2(0, 0), ImGuiDockNodeFlags_None);
		ImGui::End();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Viewport");
	ImVec2 size = ImGui::GetContentRegionAvail();

	ImGui::Image(
		(ImTextureID)(uintptr_t)m_fb->GetColorAttachment(0),
		size,
		ImVec2(0, 1),   // uv0
		ImVec2(1, 0)    // uv1
	);
	if (size.x > 0 && size.y > 0) {
		m_fb->Resize((uint32_t)size.x, (uint32_t)size.y);
	}
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::ShowDemoWindow();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorSystem::Shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (m_fb) {
		m_fb->Unbind();
	}

	delete m_fb;
	delete m_ActiveScene;

	m_fb = nullptr;
	m_ActiveScene = nullptr;
}

void EditorSystem::NewScene() {
	delete m_ActiveScene;
	m_ActiveScene = new ic::RenderScene();
	m_ScenePath = "";
	
	// _selected  = {};
	ic_set_scene(m_ActiveScene);
    	IC_CORE_INFO("Editor: new scene created");

}

void EditorSystem::SaveScene() {
	if (!m_ScenePath) {
		SaveSceneAs("assets/scenes/untitled.scene");
		return;
	}

	// make some directory
	SaveLastScenePath(m_ScenePath);
	IC_CORE_INFO("Editor: saved scene to {}", m_ScenePath);
}

void EditorSystem::SaveSceneAs(const char* path) {
	m_ScenePath = path;
	SaveScene();
}

void EditorSystem::OpenScene(const char* path) {
	if (!ic_exists(path)) {
		IC_CORE_WARN("Editor: scene not found: {}", path);
		return;
	}
	ic::SceneSerializer serializer;
	delete m_ActiveScene;
	m_ActiveScene = serializer.Deserialize(path);
	m_ScenePath  = path;
	// m_Scelectd = {};
	SaveLastScenePath(path);
	ic_set_scene(m_ActiveScene);
	IC_CORE_INFO("Editor: opened scene {}", path);
}

// These functions are used with multiple scenes and editor config with a scene for now we are working with default scne.
const char* EditorSystem::LoadLastScenePath() {
	if (!ic_exists(m_ScenePath))
		return "";
	try 
	{
		std::ifstream f(m_ScenePath);
		string line;
		while (getline(f, line)) {
			auto pos = line.find("last_scene:");
			if (pos != string::npos) {
				string val = line.substr(pos + 11);
				// trim whitespace and quotes
				auto s = val.find_first_not_of(" \t\"");
				auto e = val.find_last_not_of(" \t\"");
				if (s != std::string::npos)
					return val.substr(s, e - s + 1).c_str();
			}
		}
	}
	catch (const std::exception& e)
	{
		IC_CORE_WARN("EditorSystem: failed to read config: {}", e.what());
		return "";
	}

	return m_ScenePath;
}

void EditorSystem::SaveLastScenePath(const char* path) {
	const char* parent = fs_getParentPath(path);
	if (!fs_mkdir(parent)) {
		IC_CORE_WARN("EditorSystem: cannot create parent path");
	}
	std::ofstream f(m_ScenePath);
	f << "last_scene: \"" << path << "\"\n";
}

} // namespace ic
