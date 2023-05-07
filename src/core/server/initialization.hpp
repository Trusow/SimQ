#ifndef SIMQ_CORE_SERVER_INITIALIZATION
#define SIMQ_CORE_SERVER_INITIALIZATION

#include "access.hpp"
#include "store.hpp"
#include "changes.hpp"
#include "q/manager.hpp"
#include "logger.hpp"

namespace simq::core::server {
    class Initialization {
        private:
            Store *_store = nullptr;
            Access *_access = nullptr;
            Changes *_changes = nullptr;
            q::Manager *_q = nullptr;

            void _initGroups();
            void _initChannels( const char *group );
            void _initConsumers( const char *group, const char *channel );
            void _initProducers( const char *group, const char *channel );

        public:
            Initialization( Store &store, Access &access, Changes &changes, q::Manager &q );

            void pollChanges();
    };

    void Initialization::_initConsumers( const char *group, const char *channel ) {
        std::vector<std::string> items;
        _store->getConsumers( group, channel, items );

        for( auto i = 0; i < items.size(); i++ ) {
            auto login = items[i].c_str();

            std::list<Logger::Detail> details;
            Logger::addItemToDetails( details, "login", login );

            try {
                unsigned char password[crypto::HASH_LENGTH];
                _store->getConsumerPassword( group, channel, login, password );

                _access->addConsumer( group, channel, login, password );

                Logger::success(
                    Logger::OP_INITIALIZATION_CONSUMER,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CONSUMER,
                    err,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CONSUMER,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    Logger::I_ROOT
                );
            }
        }
    }

    void Initialization::_initProducers( const char *group, const char *channel ) {
        std::vector<std::string> items;
        _store->getProducers( group, channel, items );

        for( auto i = 0; i < items.size(); i++ ) {
            auto login = items[i].c_str();

            std::list<Logger::Detail> details;
            Logger::addItemToDetails( details, "login", login );

            try {
                unsigned char password[crypto::HASH_LENGTH];
                _store->getProducerPassword( group, channel, login, password );

                _access->addProducer( group, channel, login, password );

                Logger::success(
                    Logger::OP_INITIALIZATION_PRODUCER,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_PRODUCER,
                    err,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_PRODUCER,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    Logger::I_ROOT
                );
            }
        }
    }

    void Initialization::_initChannels( const char *group ) {
        std::vector<std::string> items;
        _store->getChannels( group, items );

        for( auto i = 0; i < items.size(); i++ ) {
            auto channel = items[i].c_str();

            std::list<Logger::Detail> details;
            Logger::addItemToDetails( details, "channel", channel );

            try {
                util::types::ChannelLimitMessages limitMessages{};
                _store->getChannelLimitMessages( group, channel, limitMessages );

                Logger::addItemToDetails(
                    details,
                    "minMessageSize",
                    std::to_string( limitMessages.minMessageSize ).c_str()
                );
                Logger::addItemToDetails(
                    details,
                    "maxMessageSize",
                    std::to_string( limitMessages.maxMessageSize ).c_str()
                );
                Logger::addItemToDetails(
                    details,
                    "maxMessagesInMemory",
                    std::to_string( limitMessages.maxMessagesInMemory ).c_str()
                );
                Logger::addItemToDetails(
                    details,
                    "maxMessagesOnDisk",
                    std::to_string( limitMessages.maxMessagesOnDisk ).c_str()
                );

                _access->addChannel( group, channel );
                //_q.addChannel( group, channel, ".", limitMessages );

                Logger::success(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    0,
                    details,
                    simq::core::server::Logger::I_ROOT
                );

                _initConsumers( group, channel );
                _initProducers( group, channel );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    err,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    Logger::I_ROOT
                );
            }
        }
    }

    void Initialization::_initGroups() {
        std::vector<std::string> items;
        _store->getGroups( items );

        for( auto i = 0; i < items.size(); i++ ) {
            auto group = items[i].c_str();

            std::list<Logger::Detail> details;
            Logger::addItemToDetails( details, "group", group );

            try {
                unsigned char password[crypto::HASH_LENGTH];

                _store->getGroupPassword( group, password );

                _access->addGroup( group, password );
                _q->addGroup( group );

                Logger::success(
                    Logger::OP_INITIALIZATION_GROUP,
                    0,
                    details,
                    simq::core::server::Logger::I_ROOT
                );

                _initChannels( group );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    err,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    Logger::I_ROOT
                );
            }
        }
    }

    Initialization::Initialization( Store &store, Access &access, Changes &changes, q::Manager &q ) {
        _store = &store;
        _access = &access;
        _changes = &changes;
        _q = &q;

        _initGroups();
    }

    void Initialization::pollChanges() {
    }

}

#endif
