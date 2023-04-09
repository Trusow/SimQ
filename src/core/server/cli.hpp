#ifndef SIMQ_CORE_SERVER_CLI
#define SIMQ_CORE_SERVER_CLI

#include "cli_callbacks.hpp"
#include <vector>
#include <string>

namespace simq::core::server {
    class CLI {
        private:
            CLICallbacks *_cb = nullptr;
        public:
            CLI( CLICallbacks *cb );
    };

    CLI::CLI( CLICallbacks *cb ) {
        _cb = cb;
    }
}

#endif
