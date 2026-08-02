module;
#include <vitaGL.h>

export module vita.ui;
import std;
import vita.math;
import vita.gfx;
import vita.process;

namespace vita::ui::fonts {
  static const uint8_t font[] = {
#embed "assets/font.png"
  };

  constexpr int asciiStart = 33;
  constexpr Vec2i atlasSize = {512, 192};
  constexpr int atlasCols = 16;
  constexpr int cellSize = 32;
  constexpr float letterSpacing = 0.7;
  // Cells 0..91 of the atlas carry ink, covering '!' through '|'. Anything
  // outside that range falls back to '?'.
  constexpr int glyphCount = 92;

  static gfx::Texture atlasTex{};

  void init() {
    auto tex = gfx::load_texture_from_image(font);
    if (!tex)
      fatal("font atlas failed to load: {}", tex.error());
    atlasTex = std::move(*tex);
  }

  std::pair<UV, UV> get_glyph_uv(char glyph) {
    int index = glyph - asciiStart;
    if (index < 0 || index >= glyphCount) {
      index = '?' - asciiStart;
    }
    int col = index % atlasCols;
    int row = index / atlasCols;

    UV texel0 = {(float)col * cellSize, (float)row * cellSize};
    UV texel1 = {texel0.u + cellSize, texel0.v + cellSize};

    UV uv0 = {texel0.u / atlasSize.x, texel0.v / atlasSize.y};
    UV uv1 = {texel1.u / atlasSize.x, texel1.v / atlasSize.y};
    return {uv0, uv1};
  }

  void draw_glyph(Vec2f pos, char glyph, Color tint, float size) {
    if (glyph == ' ')
      return;

    auto [x, y] = pos;
    auto [uv0, uv1] = get_glyph_uv(glyph);
    std::array<Vertex, 4> quad = {{
        {x, y, uv0.u, uv0.v},
        {x + size, y, uv1.u, uv0.v},
        {x + size, y + size, uv1.u, uv1.v},
        {x, y + size, uv0.u, uv1.v},
    }};

    gfx::draw_texture_quad(atlasTex, quad, tint);
  }

  void draw_text(std::string_view text, Vec2f pos, Color tint, float scale,
                 float spacing = letterSpacing) {
    const float size = cellSize * scale;
    for (auto [i, glyph] : text | std::views::enumerate)
      draw_glyph({pos.x + i * size * spacing, pos.y}, glyph, tint, size);
  }
} // namespace vita::ui::fonts

export namespace vita::ui {
  void init() { fonts::init(); }

  Vec2f measure_text(std::string_view text, float scale = 1.0f,
                     float spacing = fonts::letterSpacing) {
    return {text.size() * fonts::cellSize * spacing * scale,
            fonts::cellSize * scale};
  }

  void draw_text(std::string_view text, Vec2f pos, Color tint = Color::white(),
                 float scale = 1.0f, float spacing = fonts::letterSpacing) {
    fonts::draw_text(text, pos, tint, scale, spacing);
  }
} // namespace vita::ui
