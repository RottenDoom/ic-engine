#include <core/logger.h>
#include <core/assertions.h>

// TODO: Test
#include <platform/platform.h>

int main(void) {
    IC_FATAL("A test message: %f", 3.14f);
    IC_ERROR("A test message: %f", 3.14f);
    IC_WARN("A test message: %f", 3.14f);
    IC_INFO("A test message: %f", 3.14f);
    IC_DEBUG("A test message: %f", 3.14f);
    IC_TRACE("A test message: %f", 3.14f);

    // IC_ASSERT(1 == 0);
    platform::platform_state state;
    platform* plat = new platform(state, "IC Engine Testbed", 100, 100, 1280, 720);

    platform_shutdown(&state);
    delete plat;
    

    return 0;
}