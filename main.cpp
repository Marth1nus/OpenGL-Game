#include <engine/application.hpp>

int main()
{
  auto app = engine::application{};
  engine::startup(app);
  return app.run();
}