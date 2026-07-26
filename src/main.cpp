import std;
import vita.process;
import vita.gfx;
import vita.ctrl;
import vita.math;
import vita.ui;

using namespace vita;

constexpr int tileSize = 20;
constexpr float playerSpeed = 0.16;

struct Timer {
  float threshold;
  float accumulated = 0.0f;

  bool tick(float dt) {
    accumulated += dt;
    if (accumulated >= threshold) {
      accumulated -= threshold;
      return true;
    }
    return false;
  }
};

struct Player {
  static constexpr Vec2i up = {0, -1};
  static constexpr Vec2i right = {1, 0};
  static constexpr Vec2i down = {0, 1};
  static constexpr Vec2i left = {-1, 0};

  Vec2i vel = right, nextVel = right;
  std::deque<Vec2i> body = {{14, 10}, {13, 10}, {12, 10}, {11, 10}, {10, 10}};
  Timer moveTimer{playerSpeed};
  int score = 0;

  bool move(Vec2i foodTile, int cols, int rows) {
    vel = nextVel;
    auto newHead = body.front() + vel;

    if (newHead.x >=
        cols) { // wen need >= to avoid last col (behind the screen)
      newHead.x = 0;
    } else if (newHead.x < 0) {
      newHead.x = cols - 1;
    }

    if (newHead.y >= rows) { // we need >= to avoid lsat row (behind the screen)
      newHead.y = 0;
    } else if (newHead.y < 0) {
      newHead.y = rows - 1;
    }

    body.push_front(newHead);

    if (newHead == foodTile) {
      return true;
    }

    body.pop_back();
    return false;
  }
};

struct Food {
  Vec2i tile;
  static inline std::mt19937 rng{
      static_cast<unsigned>(vita::get_process_time_wide())};

  template <std::ranges::input_range R>
  void update(const R& reserved, int cols, int rows) {
    std::uniform_int_distribution<int> dx(0, cols - 1);
    std::uniform_int_distribution<int> dy(0, rows - 1);
    Vec2i candidate;
    do {
      candidate = {dx(rng), dy(rng)};
    } while (std::ranges::contains(reserved, candidate));
    tile = candidate;
  }
};

template <std::ranges::input_range R>
  requires std::ranges::sized_range<R>
std::vector<Vec2f> tiles_to_vertices_vector(const R& tiles, float tileSize) {
  std::vector<Vec2f> vertices;
  vertices.reserve(std::ranges::size(tiles) * 6);
  for (const auto& cell : tiles) {
    float x = cell.x * tileSize;
    float y = cell.y * tileSize;
    Vec2f tl{x, y}, tr{x + tileSize, y};
    Vec2f bl{x, y + tileSize}, br{x + tileSize, y + tileSize};
    vertices.insert(vertices.end(), {tl, tr, bl, tr, br, bl});
  }
  return vertices;
}

void rotate_quad(std::span<Vertex, 4> quad, int steps) {
  if (steps == 0)
    return;

  std::array<std::pair<float, float>, 4> uvs;
  for (int i = 0; i < 4; ++i)
    uvs[i] = {quad[i].u, quad[i].v};

  std::rotate(uvs.begin(), uvs.begin() + steps, uvs.end());

  for (int i = 0; i < 4; ++i) {
    quad[i].u = uvs[i].first;
    quad[i].v = uvs[i].second;
  }
}

namespace background {
  static const std::uint8_t image[] = {
#embed "../assets/bg.jpg"
  };
  constexpr std::array<Vertex, 4> quad = {{
      {0, 0, 0, 0},
      {screenWidth, 0, 1, 0},
      {screenWidth, screenHeight, 1, 1},
      {0, screenHeight, 0, 1},
  }};
  gfx::Texture tex{0};

  void init() { tex = gfx::load_texture_from_image(image); }

  void draw() { gfx::draw_texture_quad(tex, quad); }
} // namespace background

namespace food_sprite {
  static const std::uint8_t image[] = {
#embed "../assets/food.png"
  };
  gfx::Texture tex{0};

  void init() { tex = gfx::load_texture_from_image(image); }

  void draw(const Food& f) {
    float absX = f.tile.x * tileSize;
    float absY = f.tile.y * tileSize;
    std::array<Vertex, 4> quad = {{
        {absX, absY, 0, 0},
        {absX + tileSize, absY, 1, 0},
        {absX + tileSize, absY + tileSize, 1, 1},
        {absX, absY + tileSize, 0, 1},
    }};
    gfx::draw_texture_quad(tex, quad);
  }
} // namespace food_sprite

namespace player_sprite {
  static const std::uint8_t bodyImage[] = {
#embed "../assets/body.png"
  };
  static const std::uint8_t headImage[] = {
#embed "../assets/head.png"
  };
  static const std::uint8_t tailImage[] = {
#embed "../assets/tail.png"
  };
  static const std::uint8_t cornerImage[] = {
#embed "../assets/corner.png"
  };
  gfx::Texture bodyTex{0};
  gfx::Texture headTex{0};
  gfx::Texture tailTex{0};
  gfx::Texture cornerTex{0};
  void init() {
    bodyTex = gfx::load_texture_from_image(bodyImage);
    headTex = gfx::load_texture_from_image(headImage);
    tailTex = gfx::load_texture_from_image(tailImage);
    cornerTex = gfx::load_texture_from_image(cornerImage);
  }

  int direction_to_rotation_steps(Vec2i dir) {
    if (dir == Player::right)
      return 3;
    if (dir == Player::down)
      return 2;
    if (dir == Player::left)
      return 1;
    return 0;
  }

  int corner_rotation(Vec2i dirIn, Vec2i dirOut) {
    const Vec2i toHead = dirIn;
    const Vec2i toTail = dirOut * -1;

    Vec2i a = Player::down, b = Player::right;
    for (int steps = 0; steps < 4; ++steps) {
      if ((toHead == a && toTail == b) || (toHead == b && toTail == a))
        return steps;
      a = {a.y, -a.x}; // one CCW step
      b = {b.y, -b.x};
    }
    return 0;
  }

  void draw(const Player& p) {
    for (std::size_t i = 0; i < p.body.size(); ++i) {
      Vec2i curr = p.body[i];
      std::optional<Vec2i> prev =
          (i > 0) ? std::optional{p.body[i - 1]} : std::nullopt;
      std::optional<Vec2i> next =
          (i + 1 < p.body.size()) ? std::optional{p.body[i + 1]} : std::nullopt;

      float x = curr.x * tileSize;
      float y = curr.y * tileSize;
      Vec2i dirIn = prev ? *prev - curr : p.vel;

      std::array<Vertex, 4> quad = {{
          {x, y, 0, 0},
          {x + tileSize, y, 1, 0},
          {x + tileSize, y + tileSize, 1, 1},
          {x, y + tileSize, 0, 1},
      }};

      if (i == 0) {
        rotate_quad(quad, direction_to_rotation_steps(p.vel));
        gfx::draw_texture_quad(headTex, quad);
      } else if (!next) { // tail
        rotate_quad(quad, direction_to_rotation_steps(dirIn));
        gfx::draw_texture_quad(tailTex, quad);
      } else {
        Vec2i dirOut = curr - *next;
        if (dirIn != dirOut) {
          rotate_quad(quad, corner_rotation(dirIn, dirOut));
          gfx::draw_texture_quad(cornerTex, quad);
        } else {
          rotate_quad(quad, direction_to_rotation_steps(dirIn));
          gfx::draw_texture_quad(bodyTex, quad);
        }
      }
    }
  }
} // namespace player_sprite

int main() {
  const int cols = screenWidth / tileSize;
  const int rows = screenHeight / tileSize;

  ctrl::init();
  gfx::init_2d();
  ui::init();

  background::init();
  player_sprite::init();
  food_sprite::init();

  ctrl::Controller pad{};
  gfx::Grid g{cols, rows, tileSize};
  Player p{};
  Food food{};

  food.update(p.body, cols, rows);

  for (;;) {
    float dt = delta();
    pad.update();

    if (p.moveTimer.tick(dt)) {
      if (p.move(food.tile, cols,
                 rows)) { // true if head.curr == food.curr / eat
        food.update(p.body, cols, rows);
        p.score++;
      }
    }

    if (p.vel.x == 0) {
      if (pad.held(ctrl::Button::Right))
        p.nextVel = Player::right;
      if (pad.held(ctrl::Button::Left))
        p.nextVel = Player::left;
    }

    if (p.vel.y == 0) {
      if (pad.held(ctrl::Button::Up))
        p.nextVel = Player::up;
      if (pad.held(ctrl::Button::Down))
        p.nextVel = Player::down;
    }

    gfx::clear(Color::rgb(0.2, 0.4, 0.8));
    background::draw();
    // gfx::draw_grid(g, Color::white());
    food_sprite::draw(food);
    player_sprite::draw(p);

    ui::draw_text(std::format("score:{}", p.score), {0, 0});

    gfx::present();
  }

  return 0;
}
