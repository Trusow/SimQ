#ifndef SIMQ_UTIL_CONSOLE_CALLBACKS
#define SIMQ_UTIL_CONSOLE_CALLBACKS

#include <vector>
#include <string>

namespace simq::util {
    class ConsoleCallbacks {
        public:
            virtual void confirm( bool ) = 0;
            virtual void inputPassword( const char * ) = 0;
            virtual void input( std::vector<std::string> & ) = 0;
    };
}

#endif
