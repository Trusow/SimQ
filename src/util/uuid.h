#ifndef SIMQ_UTIL_UUID 
#define SIMQ_UTIL_UUID 

#include <random>

namespace simq::util {
    class UUID {
        private:
            static char convertIntToHex( unsigned int value );
        public:
            static const unsigned int LENGTH = 36;
            static void generate( char *data );
    };
}

#endif
