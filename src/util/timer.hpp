#ifndef SIMQ_CORE_UTIL_TIMER
#define SIMQ_CORE_UTIL_TIMER

#include <chrono>

namespace simq::util {
    class Timer {
        public:
        static unsigned long int tick();
    };

    unsigned long int Timer::tick() {
        auto time = std::chrono::system_clock::now();
        auto epoch = time.time_since_epoch();

        return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    }
}

#endif
 
