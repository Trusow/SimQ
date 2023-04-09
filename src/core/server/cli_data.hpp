#ifndef SIMQ_CORE_SERVER_CLI_DATA
#define SIMQ_CORE_SERVER_CLI_DATA

#include "cli_callbacks.hpp"
#include "store.hpp"
#include "changes.hpp"

namespace simq::core::server {
    class CLIData: public CLICallbacks {
        private:
            Store *_store = nullptr;
            Changes *_changes = nullptr;
        public:
            CLIData( Store *store, Changes *changes );
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
            void getChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings &settings
            );

            unsigned short int getPort();
            unsigned short int getCountThreads();
            void getMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            );

            void addGroup(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            void updateGroupPassword(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            void removeGroup( const char *group );

            void addChannel(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
            );
            void updateChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
            );
            void removeChannel(
                const char *group,
                const char *channel
            );

            void addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            void updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
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
                unsigned char password[crypto::HASH_LENGTH]
            );
            void updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            void removeProducer(
                const char *group,
                const char *channel,
                const char *login
            );

            void updateMasterPassword( unsigned char password[crypto::HASH_LENGTH] );
            void updatePort( unsigned short int port );
            void updateCountThreads( unsigned short int count );
    };

    CLIData::CLIData( Store *store, Changes *changes ) {
        _store = store;
        _changes = changes;
    }

    void CLIData::getGroups( std::vector<std::string> &list ) {
        _store->getDirectGroups( list );
    }

    void CLIData::getChannels( const char *group, std::vector<std::string> &list ) {
        _store->getDirectChannels( group, list );
    }

    void CLIData::getConsumers(
        const char *group,
        const char *channel,
        std::vector<std::string> &list
    ) {
        _store->getDirectConsumers( group, channel, list );
    }

    void CLIData::getProducers(
        const char *group,
        const char *channel,
        std::vector<std::string> &list
    ) {
        _store->getDirectProducers( group, channel, list );
    }

    void CLIData::getChannelSettings(
        const char *group,
        const char *channel,
        util::Types::ChannelSettings &settings
    ) {
        _store->getDirectChannelSettings( group, channel, settings );
    }

    unsigned short int CLIData::getPort() {
        return _store->getDirectPort();
    }

    unsigned short int CLIData::getCountThreads() {
        return _store->getDirectCountThreads();
    }

    void CLIData::getMasterPassword(
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _store->getDirectMasterPassword( password );
    }

    void CLIData::addGroup(
        const char *group,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->addGroup( group, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateGroupPassword(
        const char *group,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->updateGroupPassword( group, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::removeGroup( const char *group ) {
        auto ch = _changes->removeGroup( group );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::addChannel(
        const char *group,
        const char *channel,
        util::Types::ChannelSettings *settings
    ) {
        auto ch = _changes->addChannel( group, channel, settings );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateChannelSettings(
        const char *group,
        const char *channel,
        util::Types::ChannelSettings *settings
    ) {
        auto ch = _changes->updateChannelSettings( group, channel, settings );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::removeChannel(
        const char *group,
        const char *channel
    ) {
        auto ch = _changes->removeChannel( group, channel );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::addConsumer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->addConsumer( group, channel, login, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->updateConsumerPassword( group, channel, login, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::removeConsumer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        auto ch = _changes->removeConsumer( group, channel, login );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::addProducer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->addProducer( group, channel, login, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto ch = _changes->updateProducerPassword( group, channel, login, password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::removeProducer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        auto ch = _changes->removeProducer( group, channel, login );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateMasterPassword( unsigned char password[crypto::HASH_LENGTH] ) {
        auto ch = _changes->updateMasterPassword( password );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updatePort( unsigned short int port ) {
        auto ch = _changes->updatePort( port );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }

    void CLIData::updateCountThreads( unsigned short int count ) {
        auto ch = _changes->updateCountThreads( count );
        _changes->pushDefered( ch );
        _changes->free( ch );
    }
}

#endif
