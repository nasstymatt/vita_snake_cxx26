module;

export module vita.platform;
import std;
import vita.gfx;
import vita.ui;
import vita.ctrl;

export namespace vita {
  // Brings up every subsystem in the one order that works, so callers cannot
  // get it wrong: ui::init() uploads the font atlas and therefore needs a live
  // GL context, which only gfx::init_2d() can provide.
  //
  // There is deliberately no destructor. The game loop never exits, so nothing
  // would call it -- and adding one would introduce a real hazard, since it
  // would tear down GL while textures with static storage duration (the font
  // atlas) still have their own destructors pending.
  struct Platform {
    Platform() {
      gfx::init_2d();
      ui::init();
      ctrl::init();
    }

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
  };
} // namespace vita
