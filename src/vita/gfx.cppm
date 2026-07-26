module;
#include <vitaGL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
export module vita.gfx;
import std;
import vita.math;
import vita.process;

export constexpr int screenWidth = 960;
export constexpr int screenHeight = 544;

export struct Color {
  float r, g, b, a = 1.0f;

  static constexpr Color white() { return {1, 1, 1, 1}; }
  static constexpr Color black() { return {0, 0, 0, 1}; }
  static constexpr Color transparent() { return {0, 0, 0, 0}; }
  static constexpr Color rgb(float r, float g, float b) { return {r, g, b, 1}; }
};

using StbImage = std::unique_ptr<uint8_t, decltype(&stbi_image_free)>;

export namespace vita::gfx {
  enum class DrawArraysMode : GLenum {
    Lines = GL_LINES,
    Triangles = GL_TRIANGLES,
  };

  struct Texture {
    explicit Texture(uint32_t tex) : gl_tex(tex) {}
    uint32_t getGLTex() const { return gl_tex; }

  private:
    uint32_t gl_tex;
  };

  struct Grid {
    std::vector<Vec2f> vertices;

    Grid(int cols, int rows, float tileSize) {
      vertices.reserve((cols + rows + 2) * 2);

      float width = cols * tileSize;
      float height = rows * tileSize;

      for (int i = 1; i <= cols; i++) {
        vertices.insert(vertices.end(),
                        {{i * tileSize, 0}, {i * tileSize, height}});
      }

      for (int i = 1; i <= rows; i++) {
        vertices.insert(vertices.end(),
                        {{0, i * tileSize}, {width, i * tileSize}});
      }
    }
  };

  void init_2d(int legacy_pool_size = 0x800000) {
    vglInitExtended(0, 960, 544, legacy_pool_size, SCE_GXM_MULTISAMPLE_NONE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
  }

  void clear(Color c = Color::black()) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void color(Color c) { glColor4f(c.r, c.g, c.b, c.a); }

  void draw_vertices(DrawArraysMode mode, const std::span<const Vec2f> vertices,
                     Color c) {
    color(c);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices.data());
    glDrawArrays(std::to_underlying(mode), 0, vertices.size());
    glDisable(GL_VERTEX_ARRAY);
  }

  void draw_grid(const Grid& g, Color c) {
    draw_vertices(DrawArraysMode::Lines, g.vertices, c);
  }

  void draw_rects_vertices(std::span<const Vec2f> v, Color c) {
    draw_vertices(DrawArraysMode::Triangles, v, c);
  }

  void present() { vglSwapBuffers(GL_FALSE); }

  Texture load_texture(Vec2i size, std::span<const uint8_t> pixels) {
    GLuint gl_tex;

    glGenTextures(1, &gl_tex);
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return Texture(gl_tex);
  }

  Texture load_texture_from_image(std::span<const uint8_t> image_bytes) {
    int w, h, channels;
    StbImage pixels(stbi_load_from_memory(image_bytes.data(),
                                          image_bytes.size(), &w, &h, &channels,
                                          4),
                    &stbi_image_free);

    vita::println("pixels size: {}", image_bytes.size());

    if (!pixels) {
      vita::println("stb_image failed: {}", stbi_failure_reason());
      // sceClibAbort();
      // unreachable
    }

    return load_texture({w, h},
                        std::span<const uint8_t>(pixels.get(), w * h * 4));
  }

  void draw_texture_quad(const Texture& tex, std::span<const Vertex, 4> quad,
                         Color c = Color::white()) {
    color(c);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex.getGLTex());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &quad[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &quad[0].u);

    glDrawArrays(GL_QUADS, 0, 4);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY); // teardown
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
  }
} // namespace vita::gfx
