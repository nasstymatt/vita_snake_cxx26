module;

export module vita.math;
import std;

export namespace vita {
  template <typename T>
  concept Arithmetic = std::integral<T> || std::floating_point<T>;

  template <Arithmetic T> struct Vec2 {
    T x{}, y{};

    constexpr Vec2 operator*(T s) const { return {x * s, y * s}; }
    constexpr Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }

    constexpr bool operator==(const Vec2&) const = default;
    constexpr Vec2& operator+=(Vec2 o) {
      x += o.x;
      y += o.y;
      return *this;
    }
    constexpr Vec2& operator-=(Vec2 o) {
      x -= o.x;
      y -= o.y;
      return *this;
    }

    auto len() const -> std::conditional_t<std::floating_point<T>, T, double> {
      return std::sqrt(static_cast<double>(x * x + y * y));
    }
  };

  using Vec2i = Vec2<int>;
  using Vec2f = Vec2<float>;

  struct UV {
    float u, v;
  };

  struct Vertex {
    float x, y, u, v;
  };

  static_assert(std::is_standard_layout_v<Vertex>);
  static_assert(sizeof(Vertex) == 4 * sizeof(float));
} // namespace vita
