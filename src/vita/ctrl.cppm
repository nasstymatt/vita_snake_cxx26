module;
#include <psp2/ctrl.h>

export module vita.ctrl;
import std;
import vita.math;

export namespace vita::ctrl {
  enum class Button : uint32_t {
    Left = SCE_CTRL_LEFT,
    Right = SCE_CTRL_RIGHT,
    Up = SCE_CTRL_UP,
    Down = SCE_CTRL_DOWN,
    Cross = SCE_CTRL_CROSS,
    Circle = SCE_CTRL_CIRCLE,
    Start = SCE_CTRL_START,
    Select = SCE_CTRL_SELECT,
  };

  struct Controller {
    void update() {
      prev_buttons = data.buttons;
      sceCtrlPeekBufferPositive(0, &data, 1);
    }

    bool held(Button b) const { return (data.buttons & std::to_underlying(b)); }

    bool pressed(Button b) const {
      auto mask = std::to_underlying(b);
      return (data.buttons & mask) && !(prev_buttons & mask);
    }

    Vec2f ls() const {
      Vec2f s = {(static_cast<float>(data.lx) - 128.0f) / 128.0f,
                 (static_cast<float>(data.ly) - 128.0f) / 128.0f};
      if (s.len() < 0.1f)
        return {0, 0};
      return s;
    }

  private:
    SceCtrlData data{};
    uint32_t prev_buttons{};
  };

  void init() { sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); }
} // namespace vita::ctrl
