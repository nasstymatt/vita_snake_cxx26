import std;
import vita.process;
import vita.platform;
import vita.gfx;
import vita.ctrl;

import snake.assets;
import snake.game;
import snake.scene;

using namespace vita;

int main() {
  const Platform platform{};

  snake::Assets assets{};
  assets.load();

  ctrl::Controller pad{};

  const snake::Board board{screenWidth / snake::tileSize,
                           screenHeight / snake::tileSize, snake::tileSize};

  snake::Scene scene{std::in_place_type<snake::Menu>};

  for (;;) {
    pad.update();
    const snake::Context ctx{board, assets, pad, delta()};

    if (auto next =
            std::visit([&](auto& s) { return snake::update(s, ctx); }, scene))
      scene = std::move(*next);

    gfx::clear(Color::rgb(0.2, 0.4, 0.8));
    std::visit([&](const auto& s) { snake::draw(s, ctx); }, scene);
    gfx::present();
  }

  return 0;
}
