#ifndef SIMQ_CORE_SERVER_SERVER_CALLBACKS
#define SIMQ_CORE_SERVER_SERVER_CALLBACKS

namespace simq::core::server::server {
    class Callbacks {
        public:
            virtual void connect( unsigned int fd, unsigned int ip ) = 0;
            virtual void recv( unsigned int fd ) = 0;
            virtual void send( unsigned int fd ) = 0;
            virtual void disconnect( unsigned int fd ) = 0;
            virtual void polling( unsigned int delay ) = 0;
    };
}


#endif
