#include <game/boids.hpp>
#include <game/controls.hpp>
#include <game/game_of_life.hpp>

/**/ game::layers::controls::controls(std::string_view game_name)
{
  switch_game(game_name);
}
/**/ game::layers::controls::~controls()
{
}
auto game::layers::controls::switch_game(std::string_view game_name) -> void
{
  auto const it = std::ranges::find(m_games, game_name, [](std::string_view sv)
                                    { return sv; });
  runtime_assert<std::logic_error>(it != m_games.end(), "{:?} not in {}", game_name, m_games);
  m_current_game_index = -static_cast<decltype(0z)>(it - m_games.begin());
}
auto game::layers::controls::get_game() const -> std::tuple<std::string_view, size_t, bool>
{
  auto const switching = m_current_game_index < 0z;
  auto const i         = static_cast<size_t>(std::abs(m_current_game_index));
  runtime_assert<std::out_of_range>(i < m_games.size(), "out of range");
  return {m_games.at(i), i, switching};
}
auto game::layers::controls::on_update() -> update_delay
{
  auto static constexpr update_delay       = layer::update_delay(1.0);
  auto const [game, game_index, switching] = get_game();
  if (not switching) return update_delay;
  auto const self_it = std::ranges::find(app().get_layers(), this, [](auto const &p) -> layer const *
                                         { return p.get(); });
  runtime_assert(self_it != app().get_layers().end(), "Controls layers is not attached");
  auto const self = std::static_pointer_cast<controls>(*self_it);
  for (auto const &layer : m_layers)
    app().schedule_layer_pop(layer.get());
  clients.clear();
  m_layers.clear();
  m_layers = [&] -> decltype(m_layers)
  {
    if (game == "none") return {
        std::make_shared<layers::clear>(),
    };
    if (game == "clear") return {
        std::make_shared<layers::clear>(),
    };
    if (game == "boids") return {
        std::make_shared<layers::clear>(glm::vec4{0.15f, 0.05f, 0.5f, 1.0f}),
        std::make_shared<layers::boids>(),
    };
    if (game == "game of life") return {
        std::make_shared<layers::game_of_life>(),
    };
    throw std::logic_error{std::format("{:?} is not a valid game name", game)}; //
  }();
  for (auto const &layer : m_layers)
  {
    app().schedule_layer_push(layer, self.get());
    if (auto const client = std::dynamic_pointer_cast<imgui::client>(layer))
      clients.push_back(client);
  }
  m_current_game_index *= -1;
  std::println("switched to state: {:?}", game);
  return update_delay;
}
auto game::layers::controls::on_imgui() -> void
{
  auto const viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(viewport->Size, ImGuiCond_Always);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.1f);
  auto const window_flags = ImGuiWindowFlags{
      ImGuiWindowFlags_None              //
      | ImGuiWindowFlags_NoMove          //
      | ImGuiWindowFlags_NoResize        //
      | ImGuiWindowFlags_NoSavedSettings //
  };
  if (ImGui::Begin("Controls", nullptr, window_flags))
  {
    auto const [game, game_index, switching] = get_game();
    auto const combo_size                    = ImGui::CalcTextSize(" Game Of Life Current Game ");
    ImGui::SetNextItemWidth(combo_size.x);
    auto       combo_index   = static_cast<int>(game_index);
    auto const combo_changed = ImGui::Combo(switching ? "Switching Game" : "Current Game",
                                            &combo_index,
                                            m_games.data(),
                                            static_cast<int>(m_games.size()));
    if (combo_changed) switch_game(m_games.at(combo_index));
    for (auto const &client : clients) client->on_imgui();
  }
  ImGui::End();
}