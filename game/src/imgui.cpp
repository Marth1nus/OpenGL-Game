#include <game/imgui.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct game::imgui::imgui_lifetime
{
    imgui_lifetime(engine::application &app) : app{app}
    {
      auto const window = &app.get_window();

      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO &io                      = ImGui::GetIO();
      io.ConfigFlags                  |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
      io.ConfigFlags                  &= ~ImGuiConfigFlags_NavEnableGamepad; // Disable Gamepad Controls
      io.ConfigFlags                  |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
      io.ConfigFlags                  |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
      io.ConfigViewportsNoTaskBarIcon  = true;
      ImGui::StyleColorsDark();

      ImGui_ImplGlfw_InitForOpenGL(window, true);
#if defined(__EMSCRIPTEN__)
      ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif // defined( __EMSCRIPTEN__)
      ImGui_ImplOpenGL3_Init("#version 300 es");

      io.IniFilename = nullptr;
    }
    ~imgui_lifetime()
    {
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
    }
    engine::application &app;
};
auto imgui_lifetimes = std::unordered_map<engine::application const *, std::weak_ptr<game::imgui::imgui_lifetime>>{};

/**/ game::imgui::host::host(std::vector<std::shared_ptr<client>> clients)
    : clients{std::move(clients)}
{
  auto &lifetime_weak = imgui_lifetimes[&app()];
  auto  lifetime      = lifetime_weak.lock();
  if (not lifetime)
    lifetime_weak = lifetime = {
        new imgui_lifetime{app()},
        [](imgui_lifetime *p)
        { imgui_lifetimes.erase(&p->app), delete p; },
    };
  runtime_assert(m_imgui_lifetime = lifetime);
}
/**/ game::imgui::host::~host()
{
}
auto game::imgui::host::on_render() -> void
{
  // auto const window = &app().get_window();
  ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  on_imgui();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    auto const restore = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(restore);
  }
}
auto game::imgui::host::on_imgui() -> void
{
  if (ImGui::Begin("ImGui Window"))
    for (auto const &client : clients) client->on_imgui();
  ImGui::End();
}
