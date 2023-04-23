#ifndef SIMQ_UTIL_STRING
#define SIMQ_UTIL_STRING

#include <string.h>
#include <ctype.h>

namespace simq::util {
    class String {
        public:
            static void free( char *str );
            static char *copy( const char *src );
            static int find( const char *src, const char *target );
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

    int String::find( const char *line, const char *query ) {
        auto lLine = strlen( line );
        auto lQuery = strlen( query );
        auto _line = new char[lLine+1]{};
        auto _query = new char[lQuery+1]{};

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            _line[i] = toupper( line[i] );
        }

        for( unsigned int i = 0; query[i] != 0; i++ ) {
            _query[i] = toupper( query[i] );
        }

        auto res = strstr( _line, _query );

        auto offset = res != NULL ? res - _line : -1;

        delete[] _line;
        delete[] _query;

        return offset;
    }
}

#endif
