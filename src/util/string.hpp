#ifndef SIMQ_UTIL_STRING
#define SIMQ_UTIL_STRING

#include <string.h>
#include <ctype.h>
#include <memory>

namespace simq::util {
    class String {
        public:
            static int find( const char *src, const char *target );
    };

    int String::find( const char *line, const char *query ) {
        auto lLine = strlen( line );
        auto lQuery = strlen( query );
        auto _line = std::make_unique<char[]>( lLine+1 );
        auto _query = std::make_unique<char[]>( lQuery+1 );

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            _line[i] = toupper( line[i] );
        }

        for( unsigned int i = 0; query[i] != 0; i++ ) {
            _query[i] = toupper( query[i] );
        }

        auto res = strstr( _line.get(), _query.get() );

        auto offset = res != NULL ? res - _line.get() : -1;

        return offset;
    }
}

#endif
