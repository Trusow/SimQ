#ifndef SIMQ_CORE_SERVER_CLI_CMD
#define SIMQ_CORE_SERVER_CLI_CMD

#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class Cmd {
        public:
            virtual void run( std::vector<std::string> &list ) = 0;
    };

}

#endif
