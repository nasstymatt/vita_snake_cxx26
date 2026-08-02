module;

export module snake.game;
import std;
import vita.math;
import vita.process;

using namespace vita;

export namespace snake {
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

  // The playfield in tiles, plus the tile <-> pixel conversion.
  struct Board {
    int cols, rows;
    float tileSize;

    Vec2f to_pixels(Vec2i tile) const {
      return {tile.x * tileSize, tile.y * tileSize};
    }

    Vec2f tile_size() const { return {tileSize, tileSize}; }

    // Tiles past an edge come back on the opposite one. We need >= here to
    // avoid the last col/row, which sits behind the screen.
    Vec2i wrap(Vec2i t) const {
      if (t.x >= cols) {
        t.x = 0;
      } else if (t.x < 0) {
        t.x = cols - 1;
      }

      if (t.y >= rows) {
        t.y = 0;
      } else if (t.y < 0) {
        t.y = rows - 1;
      }

      return t;
    }
  };

  struct Player {
    static constexpr Vec2i up = {0, -1};
    static constexpr Vec2i right = {1, 0};
    static constexpr Vec2i down = {0, 1};
    static constexpr Vec2i left = {-1, 0};

    Vec2i vel = right, nextVel = right;
    std::deque<Vec2i> body = {{14, 10}, {13, 10}, {12, 10}};

    bool step(Vec2i foodTile, const Board& board) {
      vel = nextVel;
      auto newHead = board.wrap(body.front() + vel);

      body.push_front(newHead);

      if (newHead == foodTile) {
        return true; // ate: keep the tail, the snake grows by one
      }

      body.pop_back();
      return false;
    }

    bool self_collides() const {
      return std::ranges::contains(body | std::views::drop(1), body.front());
    }
  };

  struct Food {
    Vec2i tile;
    static inline std::mt19937 rng{
        static_cast<unsigned>(vita::get_process_time_wide())};

    template <std::ranges::input_range R>
    void update(const R& reserved, const Board& board) {
      std::uniform_int_distribution<int> dx(0, board.cols - 1);
      std::uniform_int_distribution<int> dy(0, board.rows - 1);
      Vec2i candidate;
      do {
        candidate = {dx(rng), dy(rng)};
      } while (std::ranges::contains(reserved, candidate));
      tile = candidate;
    }
  };
} // namespace snake
