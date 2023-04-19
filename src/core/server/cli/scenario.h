#ifndef SIMQ_CORE_SERVER_CLI_SCENARIO
#define SIMQ_CORE_SERVER_CLI_SCENARIO

#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class Scenario {
        public:
            virtual void start() = 0;
            virtual void input( std::vector<std::string> &list ) = 0;
            virtual void password( const char *value ) = 0;
            virtual void confirm( bool value ) = 0;
            virtual bool isEnd() = 0;
    };
}

#endif
 
