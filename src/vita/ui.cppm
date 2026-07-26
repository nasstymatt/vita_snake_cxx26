module;
#include <vitaGL.h>

export module vita.ui;
import std;
import vita.math;
import vita.gfx;

namespace vita::ui::fonts {
  static const uint8_t font[] = {
		#embed "assets/font.png"
  };
	constexpr int		asciiStart	= 33;
	constexpr Vec2i atlasSize		= {512, 192};
	constexpr int		atlasCols		= 16;
	constexpr int   cellSize		= 32;
	constexpr int		glyphCount	= 92;
  static gfx::Texture atlasTex{0};

	void init() {
		atlasTex = gfx::load_texture_from_image(font);
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

  void draw_glyph(Vec2f pos, char glyph) {
    if (glyph == ' ') return;
    auto [x, y] = pos;
    auto [uv0, uv1] = get_glyph_uv(glyph);
    std::array<Vertex, 4> quad = {{
      {x, y, uv0.u, uv0.v},
      {x + cellSize, y, uv1.u, uv0.v},
      {x + cellSize, y + cellSize, uv1.u, uv1.v},
      {x, y + cellSize, uv0.u, uv1.v},
    }};
    
		gfx::draw_texture_quad(atlasTex, quad);
  }

  void draw_text(std::string_view text, Vec2f pos, Color c = Color::white()) {
		gfx::color(c);
    for (auto [i, c] : text | std::views::enumerate) {
			fonts::draw_glyph({pos.x + i * cellSize, pos.y}, c);
		}
	}
} // namespace vita::ui::fonts

export namespace vita::ui {
	void init() { fonts::init(); }

  void draw_text(std::string_view text, Vec2f pos, Color c = Color::white()) {
    fonts::draw_text(text, pos, c);
	}
} // namespace vita::ui
