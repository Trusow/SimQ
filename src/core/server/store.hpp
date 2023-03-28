#ifndef SIMQ_CORE_SERVER_STORE 
#define SIMQ_CORE_SERVER_STORE 

#include <list>
#include <string>
#include <mutex>
#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/fs.hpp"


namespace simq::core::server {
    class Store {
        private:
            std::mutex m;
        public:
            Store( const char *path );

            void getGroups( std::list<std::string> &groups );
            void getGroupPassword( const char *group, char password[crypto::HASH_LENGTH] );
            void addGroup( const char *group, char password[crypto::HASH_LENGTH] );
            void removeGroup( const char *group );

            void getChannels( const char *group, std::list<std::string> &channels );
            void getChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void addChannel( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void updateChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void removeChannel( const char *group, const char *channel );

            void getConsumers( const char *group, const char *channel, std::list<std::string> &consumers );
            void getConsumerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void updateConsumerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void removeConsumer( const char *group, const char *channel, const char *login );

            void getProduces( const char *group, const char *channel, std::list<std::string> &producers );
            void getProducerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void updateProducerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void removeProducer( const char *group, const char *channel, const char *login );

            void getMasterPassword( char password[crypto::HASH_LENGTH] );
            void updateMasterPassword( char password[crypto::HASH_LENGTH] );

            unsigned int getCountThreads();
            void updateCountThreads( unsigned int count );

            unsigned int getPort();
            void updatePort( unsigned int port );
    };
}

#endif
