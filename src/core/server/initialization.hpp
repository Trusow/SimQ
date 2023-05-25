#ifndef SIMQ_CORE_SERVER_INITIALIZATION
#define SIMQ_CORE_SERVER_INITIALIZATION

#include "access.hpp"
#include "store.hpp"
#include "changes.hpp"
#include "q/manager.hpp"
#include "logger.hpp"
#include "../../util/types.h"
#include <memory>

namespace simq::core::server {
    class Initialization {
        private:
            std::unique_ptr<Store> _store;
            Access *_access = nullptr;
            std::unique_ptr<Changes> _changes;
            q::Manager *_q = nullptr;
            std::unique_ptr<char[]> _path;

            void _initGroups();
            void _initChannels( const char *group );
            void _initConsumers( const char *group, const char *channel );
            void _initProducers( const char *group, const char *channel );

            void _addGroup( std::unique_ptr<Changes::Change> &change );
            void _updateGroupPassword( std::unique_ptr<Changes::Change> &change );
            void _removeGroup( std::unique_ptr<Changes::Change> &change );

            void _addChannel( std::unique_ptr<Changes::Change> &change );
            void _updateChannelLimitMessagess( std::unique_ptr<Changes::Change> &change );
            void _removeChannel( std::unique_ptr<Changes::Change> &change );

            void _addConsumer( std::unique_ptr<Changes::Change> &change );
            void _updateConsumerPassword( std::unique_ptr<Changes::Change> &change );
            void _removeConsumer( std::unique_ptr<Changes::Change> &change );

            void _addProducer( std::unique_ptr<Changes::Change> &change );
            void _updateProducerPassword( std::unique_ptr<Changes::Change> &change );
            void _removeProducer( std::unique_ptr<Changes::Change> &change );

        public:
            Initialization( const char *path, Access &access, q::Manager &q );

            Changes *getChanges();
            void pollChanges();
    };

    void Initialization::_initConsumers( const char *group, const char *channel ) {
        std::list<std::string> items;
        _store->getConsumers( group, channel, items );

        for( auto it = items.begin(); it != items.end(); it++ ) {
            auto login = (*it).c_str();

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
                    util::types::Initiator::I_ROOT
                );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CONSUMER,
                    err,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CONSUMER,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            }
        }
    }

    void Initialization::_initProducers( const char *group, const char *channel ) {
        std::list<std::string> items;
        _store->getProducers( group, channel, items );

        for( auto it = items.begin(); it != items.end(); it++ ) {
            auto login = (*it).c_str();

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
                    util::types::Initiator::I_ROOT
                );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_PRODUCER,
                    err,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_PRODUCER,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            }
        }
    }

    void Initialization::_initChannels( const char *group ) {
        std::list<std::string> items;
        _store->getChannels( group, items );

        for( auto it = items.begin(); it != items.end(); it++ ) {
            auto channel = (*it).c_str();

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

                std::string pathToLimitMessages;
                util::constants::buildPathToChannelLimitMessages(
                    pathToLimitMessages,
                    _path.get(),
                    group,
                    channel
                );

                _q->addChannel( group, channel, pathToLimitMessages.c_str(), limitMessages );

                Logger::success(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );

                _initConsumers( group, channel );
                _initProducers( group, channel );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    err,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_CHANNEL,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
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
                    util::types::Initiator::I_ROOT
                );

                _initChannels( group );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    err,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    util::types::Initiator::I_ROOT
                );
            }
        }
    }

    Initialization::Initialization( const char *path, Access &access, q::Manager &q ) {
        auto l = strlen( path );
        _path = std::make_unique<char[]>( l + 1 );
        memcpy( _path.get(), path, l );
        _access = &access;
        _q = &q;

        std::list<Logger::Detail> details;

        try {
            _store = std::make_unique<Store>( _path.get() );
            _changes = std::make_unique<Changes>( _path.get() );

            Logger::success(
                Logger::OP_INITIALIZATION_SETTINGS,
                0,
                details,
                util::types::Initiator::I_ROOT
            );

            _initGroups();
        } catch( util::Error::Err err ) {
            Logger::fail(
                Logger::OP_INITIALIZATION_SETTINGS,
                err,
                0,
                details,
                util::types::Initiator::I_ROOT
            );
        } catch( ... ) {
            Logger::fail(
                Logger::OP_INITIALIZATION_SETTINGS,
                util::Error::UNKNOWN,
                0,
                details,
                util::types::Initiator::I_ROOT
            );
        }

    }

    Changes *Initialization::getChanges() {
        return _changes.get();
    }

    void Initialization::_addGroup( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto password = _changes->getPassword( change );

        _store->addGroup( group, password );
        _q->addGroup( group );
        _access->addGroup( group, password );
    }

    void Initialization::_updateGroupPassword( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto password = _changes->getPassword( change );

        _store->updateGroupPassword( group, password );
        _access->updateGroupPassword( group, password );
    }

    void Initialization::_removeGroup( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );

        _access->removeGroup( group );
        _q->removeGroup( group );
        _store->removeGroup( group );
    }

    void Initialization::_addChannel( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto limits = _changes->getChannelLimitMessages( change );
        util::types::ChannelLimitMessages l;
        l.minMessageSize = limits->minMessageSize;
        l.maxMessageSize = limits->maxMessageSize;
        l.maxMessagesInMemory = limits->maxMessagesInMemory;
        l.maxMessagesOnDisk = limits->maxMessagesOnDisk;

        _store->addChannel( group, channel, l );
        std::string path;
        util::constants::buildPathToChannelData( path, _path.get(), group, channel );
        _q->addChannel( group, channel, path.c_str(), l );
        _access->addChannel( group, channel );
    }

    void Initialization::_updateChannelLimitMessagess( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto limits = _changes->getChannelLimitMessages( change );
        util::types::ChannelLimitMessages l;
        l.minMessageSize = limits->minMessageSize;
        l.maxMessageSize = limits->maxMessageSize;
        l.maxMessagesInMemory = limits->maxMessagesInMemory;
        l.maxMessagesOnDisk = limits->maxMessagesOnDisk;

        _store->updateChannelLimitMessages( group, channel, l );
        std::string path;
        util::constants::buildPathToChannelData( path, _path.get(), group, channel );
        _q->updateChannelLimitMessages( group, channel, l );
    }

    void Initialization::_removeChannel( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );

        _access->removeChannel( group, channel );
        _q->removeChannel( group, channel );
        _store->removeChannel( group, channel );
    }

    void Initialization::_addConsumer( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );
        auto password = _changes->getPassword( change );

        _store->addConsumer( group, channel, login, password );
        _access->addConsumer( group, channel, login, password );
    }

    void Initialization::_updateConsumerPassword( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );
        auto password = _changes->getPassword( change );

        _store->updateConsumerPassword( group, channel, login, password );
        _access->updateConsumerPassword( group, channel, login, password );
    }

    void Initialization::_removeConsumer( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );

        _access->removeConsumer( group, channel, login );
        _store->removeConsumer( group, channel, login );
    }

    void Initialization::_addProducer( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );
        auto password = _changes->getPassword( change );

        _store->addProducer( group, channel, login, password );
        _access->addProducer( group, channel, login, password );
    }

    void Initialization::_updateProducerPassword( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );
        auto password = _changes->getPassword( change );

        _store->updateProducerPassword( group, channel, login, password );
        _access->updateProducerPassword( group, channel, login, password );
    }

    void Initialization::_removeProducer( std::unique_ptr<Changes::Change> &change ) {
        auto group = _changes->getGroup( change );
        auto channel = _changes->getChannel( change );
        auto login = _changes->getLogin( change );

        _access->removeProducer( group, channel, login );
        _store->removeProducer( group, channel, login );
    }

    void Initialization::pollChanges() {
        Logger::Operation operation = Logger::Operation::OP_ADD_GROUP;
        util::types::Initiator initiator = util::types::Initiator::I_ROOT;

        while( true ) {
            auto change = _changes->pop();

            if( change == nullptr ) {
                return;
            }

            initiator = _changes->getInitiator( change );
            auto iGroup = _changes->getInitiatorGroup( change );
            auto iChannel = _changes->getInitiatorChannel( change );
            auto iLogin = _changes->getInitiatorLogin( change );
            auto ip = _changes->getIP( change );

            std::list<Logger::Detail> details;
            Logger::addItemToDetails( details, "group", _changes->getGroup( change ) );

            try {
                if( _changes->isAddGroup( change ) ) {
                    operation = Logger::Operation::OP_ADD_GROUP;

                    _addGroup( change );
                } else if( _changes->isUpdateGroupPassword( change ) ) {
                    operation = Logger::Operation::OP_UPDATE_GROUP_PASSWORD;

                    _updateGroupPassword( change );
                } else if( _changes->isRemoveGroup( change ) ) {
                    operation = Logger::Operation::OP_REMOVE_GROUP;

                    _removeGroup( change );
                } else if( _changes->isAddChannel( change ) ) {
                    operation = Logger::Operation::OP_ADD_CHANNEL;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    auto limits = _changes->getChannelLimitMessages( change );
                    Logger::addItemToDetails(
                        details,
                        "minMessageSize",
                        std::to_string( limits->minMessageSize ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessageSize",
                        std::to_string( limits->maxMessageSize ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessagesInMemory",
                        std::to_string( limits->maxMessagesInMemory ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessagesOnDisk",
                        std::to_string( limits->maxMessagesOnDisk ).c_str()
                    );

                    _addChannel( change );
                } else if( _changes->isUpdateChannelLimitMessages( change ) ) {
                    operation = Logger::Operation::OP_UPDATE_CHANNEL_LIMIT_MESSAGES;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    auto limits = _changes->getChannelLimitMessages( change );
                    Logger::addItemToDetails(
                        details,
                        "minMessageSize",
                        std::to_string( limits->minMessageSize ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessageSize",
                        std::to_string( limits->maxMessageSize ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessagesInMemory",
                        std::to_string( limits->maxMessagesInMemory ).c_str()
                    );
                    Logger::addItemToDetails(
                        details,
                        "maxMessagesOnDisk",
                        std::to_string( limits->maxMessagesOnDisk ).c_str()
                    );

                    _updateChannelLimitMessagess( change );
                } else if( _changes->isRemoveChannel( change ) ) {
                    operation = Logger::Operation::OP_REMOVE_CHANNEL;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    _removeChannel( change );
                } else if( _changes->isAddConsumer( change ) ) {
                    operation = Logger::Operation::OP_ADD_CONSUMER;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _addConsumer( change );
                } else if( _changes->isUpdateConsumerPassword( change ) ) {
                    operation = Logger::Operation::OP_UPDATE_CONSUMER_PASSWORD;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _updateConsumerPassword( change );
                } else if( _changes->isRemoveConsumer( change ) ) {
                    operation = Logger::Operation::OP_REMOVE_CONSUMER;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _removeConsumer( change );
                } else if( _changes->isAddProducer( change ) ) {
                    operation = Logger::Operation::OP_ADD_PRODUCER;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _addProducer( change );
                } else if( _changes->isUpdateProducerPassword( change ) ) {
                    operation = Logger::Operation::OP_UPDATE_PRODUCER_PASSWORD;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _updateProducerPassword( change );
                } else if( _changes->isRemoveProducer( change ) ) {
                    operation = Logger::Operation::OP_REMOVE_PRODUCER;

                    Logger::addItemToDetails( details, "channel", _changes->getChannel( change ) );
                    Logger::addItemToDetails( details, "login", _changes->getLogin( change ) );

                    _removeProducer( change );
                }

                Logger::success(
                    operation,
                    ip,
                    details,
                    initiator,
                    iGroup, iChannel, iLogin
                );
            } catch( util::Error::Err err ) {
                Logger::fail(
                    operation,
                    err,
                    ip,
                    details,
                    initiator,
                    iGroup, iChannel, iLogin
                );
            } catch( ... ) {
                Logger::fail(
                    operation,
                    util::Error::UNKNOWN,
                    ip,
                    details,
                    initiator,
                    iGroup, iChannel, iLogin
                );
            }
        }
    }

}

#endif
