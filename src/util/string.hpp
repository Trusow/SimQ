#ifndef SIMQ_UTIL_STRING
#define SIMQ_UTIL_STRING

#include <string.h>

namespace simq::util {
    class String {
        public:
            static void free( char *str );
            static char *copy( const char *src );
    };

    void String::free( char *str ) {
        if( str != nullptr ) {
            delete[] str;
            str = nullptr;
        }
    }

    char *String::copy( const char *src ) {
        if( src == nullptr ) {
            return nullptr;
        }

        auto l = strlen( src );
        auto target = new char[l+1]{};
        memcpy( target, src, l );
        return target;
    }
}

#endif
