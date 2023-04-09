#ifndef SIMQ_CORE_SERVER_CLI_CALLBACKS
#define SIMQ_CORE_SERVER_CLI_CALLBACKS

#include <vector>
#include <string>
#include "../../util/types.h"
#include "../../crypto/hash.hpp"

namespace simq::core::server {
    class CLICallbacks {
        public:
            virtual void getGroups( std::vector<std::string> &list ) = 0;
            virtual void getChannels( const char *group, std::vector<std::string> &list ) = 0;
            virtual void getConsumers(
                const char *group,
                const char *channel,
                std::vector<std::string> &list
            ) = 0;
            virtual void getProducers(
                const char *group,
                const char *channel,
                std::vector<std::string> &list
            ) = 0;
            virtual void getChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
            ) = 0;

            virtual unsigned short int getPort() = 0;
            virtual unsigned short int getCountThreads() = 0;
            virtual void getMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;

            virtual void addGroup(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void updateGroupPassword(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void removeGroup(
                const char *group
            ) = 0;

            virtual void addChannel(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
            ) = 0;
            virtual void updateChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
            ) = 0;
            virtual void removeChannel(
                const char *group,
                const char *channel
            ) = 0;

            virtual void addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void removeConsumer(
                const char *group,
                const char *channel,
                const char *login
            ) = 0;

            virtual void addProducer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void removeProducer(
                const char *group,
                const char *channel,
                const char *login
            ) = 0;

            virtual void updateMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            ) = 0;
            virtual void updatePort( unsigned short int port ) = 0;
            virtual void updateCountThreads( unsigned short int port ) = 0;
    };
}

#endif
