module;
#include <vitaGL.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>

export module vita.process;
import std;

export namespace vita {
	uint_fast64_t get_process_time_wide() { return sceKernelGetProcessTimeWide(); }

	float delta() {
		static uint64_t prev_time = 0;
		auto now = get_process_time_wide();
		float dt = (prev_time == 0) ? 0.0f : (now - prev_time) / 1'000'000.0f;
		prev_time = now;
		return dt;
  }

  template <typename... Args>
	void println(std::format_string<Args...> fmt, Args&& ...args) {
    sceClibPrintf("%s\n",
                  std::format(fmt, std::forward<Args>(args)...).c_str());
	}
} // namespace vita
