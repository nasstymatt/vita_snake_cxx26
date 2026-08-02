module;
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <vitaGL.h>

export module vita.process;
import std;

export namespace vita {
  uint_fast64_t get_process_time_wide() {
    return sceKernelGetProcessTimeWide();
  }

  float delta() {
    static uint64_t prev_time = 0;
    auto now = get_process_time_wide();
    float dt = (prev_time == 0) ? 0.0f : (now - prev_time) / 1'000'000.0f;
    prev_time = now;
    return dt;
  }

  template <typename... Args>
  void println(std::format_string<Args...> fmt, Args&&... args) {
    sceClibPrintf("%s\n",
                  std::format(fmt, std::forward<Args>(args)...).c_str());
  }

  template <typename... Args>
  [[noreturn]] void fatal(std::format_string<Args...> fmt, Args&&... args) {
    vita::println("fatal: {}", std::format(fmt, std::forward<Args>(args)...));
    sceKernelExitProcess(1);
    __builtin_unreachable();
  }

  [[noreturn]] void quit() {
    sceKernelExitProcess(0);
    __builtin_unreachable();
  }
} // namespace vita
