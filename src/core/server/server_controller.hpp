#ifndef SIMQ_CORE_SERVER_SERVER_CONTROLLER
#define SIMQ_CORE_SERVER_SERVER_CONTROLLER

#include <iostream>
#include "server/callbacks.h"

namespace simq::core::server {
    class ServerController: public server::Callbacks {
        public:
            void connect( unsigned int fd, unsigned int ip );
            void recv( unsigned int fd );
            void send( unsigned int fd );
            void disconnect( unsigned int fd );
            void iteration();
    };

    void ServerController::connect( unsigned int fd, unsigned int ip ) {
        std::cout << "connect" << std::endl;
    }

    void ServerController::recv( unsigned int fd ) {
        std::cout << "recv" << std::endl;
    }

    void ServerController::send( unsigned int fd ) {
        std::cout << "send" << std::endl;
    }

    void ServerController::disconnect( unsigned int fd ) {
        std::cout << "disconnect" << std::endl;
    }

    void ServerController::iteration() {
        std::cout << "iteration" << std::endl;
    }
}

#endif
 
