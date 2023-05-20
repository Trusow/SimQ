#ifndef SIMQ_CORE_SERVER_CLI_CONTROLLER
#define SIMQ_CORE_SERVER_CLI_CONTROLLER

#include "./cli/callbacks.hpp"
#include "store.hpp"
#include "changes.hpp"

namespace simq::core::server {
    class CLIController: public CLI::Callbacks {
        private:
            Store *_store = nullptr;
            Changes *_changes = nullptr;
        public:
            CLIController( Store *store, Changes *changes );
            void getGroups( std::vector<std::string> &list );
            void getChannels( const char *group, std::vector<std::string> &list );
            void getConsumers(
                const char *group,
                const char *channel,
                std::vector<std::string> &list
            );
            void getProducers(
                const char *group,
                const char *channel,
                std::vector<std::string> &list
            );
            void getChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages &limitMessages
            );

            unsigned short int getPort();
            unsigned short int getCountThreads();
            void getMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            );

            void addGroup(
                const char *group,
                const unsigned char *password
            );
            void updateGroupPassword(
                const char *group,
                const unsigned char *password
            );
            void removeGroup( const char *group );

            void addChannel(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            void updateChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            void removeChannel(
                const char *group,
                const char *channel
            );

            void addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void removeConsumer(
                const char *group,
                const char *channel,
                const char *login
            );

            void addProducer(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void removeProducer(
                const char *group,
                const char *channel,
                const char *login
            );

            void updateMasterPassword( const unsigned char *password );
            void updatePort( unsigned short int port );
            void updateCountThreads( unsigned short int count );
    };

    CLIController::CLIController( Store *store, Changes *changes ) {
        _store = store;
        _changes = changes;
    }

    void CLIController::getGroups( std::vector<std::string> &list ) {
        _store->getDirectGroups( list );
    }

    void CLIController::getChannels( const char *group, std::vector<std::string> &list ) {
        _store->getDirectChannels( group, list );
    }

    void CLIController::getConsumers(
        const char *group,
        const char *channel,
        std::vector<std::string> &list
    ) {
        _store->getDirectConsumers( group, channel, list );
    }

    void CLIController::getProducers(
        const char *group,
        const char *channel,
        std::vector<std::string> &list
    ) {
        _store->getDirectProducers( group, channel, list );
    }

    void CLIController::getChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        _store->getDirectChannelLimitMessages( group, channel, limitMessages );
    }

    unsigned short int CLIController::getPort() {
        return _store->getDirectPort();
    }

    unsigned short int CLIController::getCountThreads() {
        return _store->getDirectCountThreads();
    }

    void CLIController::getMasterPassword(
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _store->getDirectMasterPassword( password );
    }

    void CLIController::addGroup(
        const char *group,
        const unsigned char *password
    ) {
        auto ch = _changes->addGroup( group, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::updateGroupPassword(
        const char *group,
        const unsigned char *password
    ) {
        auto ch = _changes->updateGroupPassword( group, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::removeGroup( const char *group ) {
        auto ch = _changes->removeGroup( group );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::addChannel(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        auto ch = _changes->addChannel( group, channel, limitMessages );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::updateChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        auto ch = _changes->updateChannelLimitMessages( group, channel, limitMessages );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::removeChannel(
        const char *group,
        const char *channel
    ) {
        auto ch = _changes->removeChannel( group, channel );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::addConsumer(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        auto ch = _changes->addConsumer( group, channel, login, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::updateConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        auto ch = _changes->updateConsumerPassword( group, channel, login, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::removeConsumer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        auto ch = _changes->removeConsumer( group, channel, login );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::addProducer(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        auto ch = _changes->addProducer( group, channel, login, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::updateProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        auto ch = _changes->updateProducerPassword( group, channel, login, password );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::removeProducer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        auto ch = _changes->removeProducer( group, channel, login );
        _changes->pushDefered( std::move( ch ) );
    }

    void CLIController::updateMasterPassword( const unsigned char *password ) {
        _store->updateMasterPassword( password );
    }

    void CLIController::updatePort( unsigned short int port ) {
        _store->updatePort( port );
    }

    void CLIController::updateCountThreads( unsigned short int count ) {
        _store->updateCountThreads( count );
    }
}

#endif
