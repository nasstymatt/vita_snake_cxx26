module;

export module snake.scene;
import std;
import vita.math;
import vita.gfx;
import vita.ctrl;
import vita.ui;
import snake.game;
import snake.assets;
import snake.render;
import vita.process;

using namespace vita;

namespace snake {
  using namespace std::literals::string_view_literals;

  export enum class MainMenuItem { Play, Quit, Count };
  export enum class PauseMenuItem { Continue, MainMenu, Count };
  export enum class GameOverMenuItem { Restart, MainMenu, Count };

  constexpr std::array menuLabels = {"PLAY"sv, "QUIT"sv};
  static_assert(menuLabels.size() == std::to_underlying(MainMenuItem::Count),
                "MainMenuItem enum and menuLabels array are out of sync!");

  constexpr std::array pauseLabels = {"CONTINUE"sv, "MAIN MENU"sv};
  static_assert(pauseLabels.size() == std::to_underlying(PauseMenuItem::Count),
                "PauseMenuItem enum and pauseLabels array are out of sync!");

  constexpr std::array gameOverLabels = {"RESTART"sv, "MAIN MENU"sv};
  static_assert(
      gameOverLabels.size() == std::to_underlying(GameOverMenuItem::Count),
      "GameOverMenuItem enum and gameOverLabels array are out of sync!");
} // namespace snake

export namespace snake {
  // Everything a scene is allowed to touch, handed to it each frame
  struct Context {
    Board board;
    Assets& assets;
    ctrl::Controller& pad;
    float dt;
  };

  struct Menu {
    MainMenuItem selected = MainMenuItem::Play;
  };

  struct Playing {
    Player player;
    Food food;
    Timer moveTimer{playerSpeed};
    int score = 0;

    explicit Playing(const Board& board) { food.update(player.body, board); }
  };

  struct Paused {
    Playing playing;
    PauseMenuItem selected = PauseMenuItem::Continue;
  };

  struct GameOver {
    Playing playing;
    GameOverMenuItem selected = GameOverMenuItem::Restart;
  };

  using Scene = std::variant<Menu, Playing, GameOver, Paused>;

  std::optional<Scene> update(Menu& scene, const Context& ctx) {
    auto next = std::to_underlying(scene.selected);

    if (ctx.pad.pressed(ctrl::Button::Up))
      next--;
    else if (ctx.pad.pressed(ctrl::Button::Down))
      next++;

    next = std::clamp(next, 0, std::ssize(menuLabels) - 1);
    scene.selected = static_cast<MainMenuItem>(next);

    if (ctx.pad.pressed(ctrl::Button::Cross)) {
      using enum MainMenuItem;
      switch (scene.selected) {
      case Play:
        return Playing{ctx.board};
      case Quit:
        quit();
      case Count:
        std::unreachable();
      }
    }
    return std::nullopt;
  }

  std::optional<Scene> update(Playing& scene, const Context& ctx) {
    if (scene.moveTimer.tick(ctx.dt)) {
      const bool ate = scene.player.step(scene.food.tile, ctx.board);
      if (ate) {
        scene.food.update(scene.player.body, ctx.board);
        scene.score++;
      }
      if (scene.player.self_collides()) {
        return GameOver{.playing = std::move(scene)};
      }
    }

    // A turn is only accepted on the axis the snake is not travelling along,
    // which is what stops it from reversing into its own neck.
    if (scene.player.vel.x == 0) {
      if (ctx.pad.held(ctrl::Button::Right))
        scene.player.nextVel = Player::right;
      if (ctx.pad.held(ctrl::Button::Left))
        scene.player.nextVel = Player::left;
    }

    if (scene.player.vel.y == 0) {
      if (ctx.pad.held(ctrl::Button::Up))
        scene.player.nextVel = Player::up;
      if (ctx.pad.held(ctrl::Button::Down))
        scene.player.nextVel = Player::down;
    }

    if (ctx.pad.pressed(ctrl::Button::Start)) {
      return Paused{.playing = std::move(scene)};
    }

    return std::nullopt;
  }

  std::optional<Scene> update(Paused& scene, const Context& ctx) {
    auto next = std::to_underlying(scene.selected);

    if (ctx.pad.pressed(ctrl::Button::Up))
      next--;
    else if (ctx.pad.pressed(ctrl::Button::Down))
      next++;

    next = std::clamp(next, 0, std::ssize(pauseLabels) - 1);
    scene.selected = static_cast<PauseMenuItem>(next);

    if (ctx.pad.pressed(ctrl::Button::Start)) {
      return std::move(scene.playing);
    }

    if (ctx.pad.pressed(ctrl::Button::Cross)) {
      using enum PauseMenuItem;
      switch (scene.selected) {
      case Continue:
        return std::move(scene.playing);
      case MainMenu:
        // TODO: Implement quit path
        return Menu{};
      case Count:
        std::unreachable();
      }
    }

    return std::nullopt;
  }

  std::optional<Scene> update(GameOver& scene, const Context& ctx) {
    auto next = std::to_underlying(scene.selected);

    if (ctx.pad.pressed(ctrl::Button::Up))
      next--;
    else if (ctx.pad.pressed(ctrl::Button::Down))
      next++;

    next = std::clamp(next, 0, std::ssize(gameOverLabels) - 1);
    scene.selected = static_cast<GameOverMenuItem>(next);

    if (ctx.pad.pressed(ctrl::Button::Cross)) {
      using enum GameOverMenuItem;
      switch (scene.selected) {
      case Restart:
        return Playing{ctx.board};
      case MainMenu:
        return Menu{};
      case Count:
        std::unreachable();
      }
    }
    return std::nullopt;
  }

  void draw(const Menu& scene, const Context& ctx) {
    render::draw_background(ctx.assets);

    for (auto [i, item] : menuLabels | std::views::enumerate) {
      auto itemSize = ui::measure_text(item);
      float y = screenHeight / 2 + i * itemSize.y;
      Color tint = (i == std::to_underlying(scene.selected))
                       ? Color::rgb(0, 0, 1)
                       : Color::white();
      ui::draw_text(item, {(screenWidth / 2 - itemSize.x / 2), y}, tint);
    }
  }

  void draw(const Playing& scene, const Context& ctx) {
    render::draw_background(ctx.assets);
    render::draw(scene.food, ctx.board, ctx.assets);
    render::draw(scene.player, ctx.board, ctx.assets);

    ui::draw_text(std::format("score:{}", scene.score), {0, 0});
  }

  void draw(const Paused& scene, const Context& ctx) {
    draw(scene.playing, ctx);
    gfx::fill_rect({0, 0}, {screenWidth, screenHeight},
                   Color::rgba(.5, .5, .5, .6));

    for (auto [i, item] : pauseLabels | std::views::enumerate) {
      auto itemSize = ui::measure_text(item);
      float y = screenHeight / 2 + i * itemSize.y;
      Color tint = (i == std::to_underlying(scene.selected))
                       ? Color::rgb(0, 0, 1)
                       : Color::white();
      ui::draw_text(item, {(screenWidth / 2 - itemSize.x / 2), y}, tint);
    }
  }

  void draw(const GameOver& scene, const Context& ctx) {
    draw(scene.playing, ctx);
    gfx::fill_rect({0, 0}, {screenWidth, screenHeight},
                   Color::rgba(.5, .5, .5, .6));

    float offsetTop = screenHeight / 2 - 50;

    std::string_view title = "GAME OVER!";
    Vec2f titleSize = ui::measure_text(title);

    std::string score = std::format("Your score: {}", scene.playing.score);
    Vec2f scoreSize = ui::measure_text(score);

    ui::draw_text(title, {screenWidth / 2 - titleSize.x / 2, offsetTop});
    offsetTop += titleSize.y;

    ui::draw_text(score, {screenWidth / 2 - scoreSize.x / 2, offsetTop});
    offsetTop += scoreSize.y * 2;

    for (auto [i, item] : gameOverLabels | std::views::enumerate) {
      auto itemSize = ui::measure_text(item);
      float y = offsetTop + i * itemSize.y;
      Color tint = (i == std::to_underlying(scene.selected))
                       ? Color::rgb(0, 0, 1)
                       : Color::white();
      ui::draw_text(item, {(screenWidth / 2 - itemSize.x / 2), y}, tint);
    }
  }
} // namespace snake
