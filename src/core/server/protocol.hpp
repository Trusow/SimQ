#ifndef SIMQ_CORE_SERVER_PROTOCOL
#define SIMQ_CORE_SERVER_PROTOCOL

#include <list>
#include <string>
#include "../../util/types.h"

// NO SAFE THREAD!!!

namespace simq::core::server {
    class Protocol {
        private:
            const unsigned int VERSION = 1'000'001;
        public:
            struct Message {
            };

            void join( unsigned int fd );
            void leave( unsigned int fd );

            void recv( unsigned int fd );
            bool isReceived( unsigned int fd );

            const Message *getReceivedMessage( unsigned int fd );

            bool isGetVersion( Message *msg );

            bool isOk( Message *message );

            bool isAuthGroup( Message *msg );
            bool isAuthConsumer( Message *msg );
            bool isAuthProducer( Message *msg );

            bool isGetChannels( Message *msg );
            bool isGetConsumers( Message *msg );
            bool isGetProducers( Message *msg );

            bool isAddChannel( Message *msg );
            bool isUpdateChannelLimitMessages( Message *msg );
            bool isRemoveChannel( Message *msg );

            bool isAddConsumer( Message *msg );
            bool isUpdateConsumerPassword( Message *msg );
            bool isRemoveConsumer( Message *msg );

            bool isAddProducer( Message *msg );
            bool isUpdateProducerPassword( Message *msg );
            bool isRemoveProducer( Message *msg );

            bool isWrongCommand( Message *msg );

            const char *getGroup( Message *msg );
            const char *getChannel( Message *msg );
            const char *getLogin( Message *msg );
            void getChannelLimitMessages(
                Message *msg,
                util::types::ChannelLimitMessages &limitMessages
            );

            bool sendVersion( unsigned int fd );

            bool sendOk( unsigned int fd );
            bool sendError( unsigned int fd, const char *description );
            bool sendStringList( unsigned int fd, std::list<std::string> &list );

            bool isPushMessageMeta( Message *msg );
            bool isPushPublicMessageMeta( Message *msg );
            bool isPushReplicaMessageMeta( Message *msg );
            unsigned int getLengthMessage( Message *msg );
            const char *getUUID( Message *msg );
            // sendOk | sendError
            bool isPushMessageBodyPart( Message *msg );
            // sendOk | sendError
            
            bool isPopMessage( Message *msg );
            bool sendMessageMeta( unsigned int fd, unsigned int length, const char *uuid );
            bool sendPublicMessageMeta( unsigned int fd, unsigned int length );
            // isOk
            // send buffer
            // isOk
            // sendOk | sendError
            // isOk

            void send( unsigned int fd );
            void isSent( unsigned int fd );
    };
}

#endif
 
