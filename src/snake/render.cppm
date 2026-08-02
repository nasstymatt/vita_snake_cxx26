module;

export module snake.render;
import std;
import vita.math;
import vita.gfx;
import snake.game;
import snake.assets;

using namespace vita;

namespace snake::render {
  // head.png faces up, tail.png connects upward, body.png runs vertically --
  // so an unrotated sprite points up, and each step turns it 90 degrees CCW.
  int direction_to_rotation_steps(Vec2i dir) {
    if (dir == Player::right)
      return 3;
    if (dir == Player::down)
      return 2;
    if (dir == Player::left)
      return 1;
    return 0;
  }

  // corner.png connects the down and right edges of the tile, and one rotation
  // step turns it 90 degrees CCW (down->right->up->left), so step k connects
  // {rot(down,k), rot(right,k)}. The neighbours to link up are the tiles the
  // segment actually touches: prev at dirIn, next at -dirOut.
  int corner_rotation(Vec2i dirIn, Vec2i dirOut) {
    const Vec2i toHead = dirIn;
    const Vec2i toTail = -dirOut;

    Vec2i a = Player::down, b = Player::right;
    for (int steps = 0; steps < 4; ++steps) {
      if ((toHead == a && toTail == b) || (toHead == b && toTail == a))
        return steps;
      a = {a.y, -a.x}; // one CCW step
      b = {b.y, -b.x};
    }
    return 0;
  }
} // namespace snake::render

export namespace snake::render {
  void draw_background(const Assets& assets) {
    assets.background.draw({0, 0}, {screenWidth, screenHeight});
  }

  void draw(const Food& food, const Board& board, const Assets& assets) {
    assets.food.draw(board.to_pixels(food.tile), board.tile_size());
  }

  void draw(const Player& player, const Board& board, const Assets& assets) {
    for (std::size_t i = 0; i < player.body.size(); ++i) {
      Vec2i curr = player.body[i];
      std::optional<Vec2i> prev =
          (i > 0) ? std::optional{player.body[i - 1]} : std::nullopt;
      std::optional<Vec2i> next = (i + 1 < player.body.size())
                                      ? std::optional{player.body[i + 1]}
                                      : std::nullopt;

      Vec2f pos = board.to_pixels(curr);
      Vec2f size = board.tile_size();
      Vec2i dirIn = prev ? *prev - curr : player.vel;

      if (i == 0) {
        assets.head.draw(pos, size, direction_to_rotation_steps(player.vel));
      } else if (!next) {
        assets.tail.draw(pos, size, direction_to_rotation_steps(dirIn));
      } else {
        Vec2i dirOut = curr - *next;
        if (dirIn != dirOut) {
          assets.corner.draw(pos, size, corner_rotation(dirIn, dirOut));
        } else {
          assets.body.draw(pos, size, direction_to_rotation_steps(dirIn));
        }
      }
    }
  }
} // namespace snake::render
