#ifndef SIMQ_UTIL_LOCK 
#define SIMQ_UTIL_LOCK 

#include <mutex>


namespace simq::util {
    class Lock {
        private:
            std::mutex *_m = nullptr;
        public:
            Lock();
            Lock( std::mutex &m );
            ~Lock();
            void lock( std::mutex &m );
    };
}

#endif
