#ifndef SIMQ_UTIL_SYNC 
#define SIMQ_UTIL_SYNC 

#include <mutex>
#include <string.h>
#include <stdlib.h>

namespace simq::util {
    class Sync {
        private:
            inline static std::mutex m;

            struct Mutex {
                char *name;
                std::mutex m;
            };

            inline static Mutex **list = nullptr;
            inline static unsigned int listLength = 0;

            static Mutex *getNewMutex( const char *name );
        public:
            static std::mutex &get( const char *name );
    };
}

#endif
