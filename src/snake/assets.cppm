module;

export module snake.assets;
import std;
import vita.math;
import vita.gfx;
import vita.process;

using namespace vita;

namespace snake::bytes {
  extern const std::uint8_t background[] = {
#embed "assets/bg.jpg"
  };
  extern const std::uint8_t head[] = {
#embed "assets/head.png"
  };
  extern const std::uint8_t body[] = {
#embed "assets/body.png"
  };
  extern const std::uint8_t tail[] = {
#embed "assets/tail.png"
  };
  extern const std::uint8_t corner[] = {
#embed "assets/corner.png"
  };
  extern const std::uint8_t food[] = {
#embed "assets/food.png"
  };
} // namespace snake::bytes

export namespace snake {
  struct Sprite {
    std::span<const std::uint8_t> bytes;
    gfx::Texture tex{};

    void load() {
      auto loaded = gfx::load_texture_from_image(bytes);
      if (!loaded)
        fatal("sprite failed to load: {}", loaded.error());
      tex = std::move(*loaded);
    }

    // rotSteps turns the sprite in 90-degree counter-clockwise steps by
    // cycling the UVs across the four (fixed) corner positions.
    void draw(Vec2f pos, Vec2f size, int rotSteps = 0,
              Color tint = Color::white()) const {
      std::array<Vertex, 4> quad = {{
          {pos.x, pos.y, 0, 0},
          {pos.x + size.x, pos.y, 1, 0},
          {pos.x + size.x, pos.y + size.y, 1, 1},
          {pos.x, pos.y + size.y, 0, 1},
      }};

      if (rotSteps != 0) {
        std::array<UV, 4> uvs;
        for (int i = 0; i < 4; ++i)
          uvs[i] = {quad[i].u, quad[i].v};

        std::rotate(uvs.begin(), uvs.begin() + rotSteps, uvs.end());

        for (int i = 0; i < 4; ++i) {
          quad[i].u = uvs[i].u;
          quad[i].v = uvs[i].v;
        }
      }

      gfx::draw_texture_quad(tex, quad, tint);
    }
  };

  // Every image the game owns. Adding one means: an #embed above, a member
  // here, and an entry in all()
  struct Assets {
    Sprite background{bytes::background};
    Sprite head{bytes::head};
    Sprite body{bytes::body};
    Sprite tail{bytes::tail};
    Sprite corner{bytes::corner};
    Sprite food{bytes::food};

    // Call once, after gfx::init_2d().
    void load() {
      for (Sprite* s : all())
        s->load();
    }

  private:
    std::array<Sprite*, 6> all() {
      return {&background, &head, &body, &tail, &corner, &food};
    }
  };
} // namespace snake
