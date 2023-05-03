#ifndef SIMQ_CORE_SERVER_Q_MANAGER
#define SIMQ_CORE_SERVER_Q_MANAGER

#include <mutex>
#include <shared_mutex>
#include <map>
#include <string>
#include <string.h>
#include <list>
#include <atomic>
#include <iterator>
#include <algorithm>
#include "../../../util/lock_atomic.hpp"
#include "../../../util/error.h"
#include "../../../util/types.h"
#include "../../../util/uuid.hpp"
#include "../../../util/buffer.hpp"
#include "uuid.hpp"
#include "messages.hpp"

namespace simq::core::server::q {
    class Manager {
        private:
            struct Channel {
                std::shared_timed_mutex mConsumers;
                std::shared_timed_mutex mProducers;
                std::atomic_uint countConsumersWrited;
                std::atomic_uint countProducersWrited;
                // public messages
                std::map<unsigned int, std::list<unsigned int>> consumers;
                std::map<unsigned int, bool> producers;

                UUID uuid;
                Messages *messages;

                std::shared_timed_mutex mQList;
                std::atomic_uint countQListWrited;
                std::list<unsigned int> QList;
            };

            struct Group {
                std::shared_timed_mutex mChannels;
                std::atomic_uint countChannelsWrited;
                std::map<std::string, Channel *> channels;
            };

            std::shared_timed_mutex _mGroups;
            std::atomic_uint _countGroupsWrited{0};

            std::map<std::string, Group *> _groups;

            void _wait( std::atomic_uint &atom );
            unsigned int _calculateTotalConsumersByMessagedID(
                std::map<unsigned int, std::list<unsigned int>> &map,
                unsigned int msgID
            );

            bool _isConsumer( std::map<unsigned int, std::list<unsigned int>> &map, unsigned int fd );
            bool _isProducer( std::map<unsigned int, bool> &map, unsigned int fd );

            void _checkConsumer( std::map<unsigned int, std::list<unsigned int>> &map, unsigned int fd );
            void _checkProducer( std::map<unsigned int, bool> &map, unsigned int fd );
        public:
            void addGroup( const char *groupName );
            void addChannel(
                const char *groupName,
                const char *channelName,
                const char *path,
                util::Types::ChannelSettings &settings
            );
            void updateChannelSettings(
                const char *groupName,
                const char *channelName,
                util::Types::ChannelSettings &settings
            );
            void removeGroup( const char *groupName );
            void removeChannel( const char *groupName, const char *channelName );

            void joinConsumer( const char *groupName, const char *channelName, unsigned int fd );
            void leaveConsumer( const char *groupName, const char *channelName, unsigned int fd );
            void joinProducer( const char *groupName, const char *channelName, unsigned int fd );
            void leaveProducer( const char *groupName, const char *channelName, unsigned int fd );

            unsigned int createMessageForQ(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int length,
                Messages::WRData &wrData,
                char *uuid
            );

            unsigned int createMessageForBroadcast(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int length,
                Messages::WRData &wrData
            );

            unsigned int createMessageForReplication(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int length,
                Messages::WRData &wrData,
                const char *uuid
            );

            void removeMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id
            );
            void removeMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                const char *uuid
            );

            void recv(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id,
                Messages::WRData &wrData
            );
            void send(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id,
                Messages::WRData &wrData
            );

            void pushMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id
            );
            unsigned int popMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                char *uuid
            );
            void revertMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id
            );
    };

    void Manager::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void Manager::addGroup( const char *groupName ) {
        util::LockAtomic lockAtomic( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _mGroups );

        auto it = _groups.find( groupName );
        if( it != _groups.end() ) {
            throw util::Error::DUPLICATE_GROUP;
        }

        auto group = new Group{};
        _groups[groupName] = group;
    }

    void Manager::removeGroup( const char *groupName ) {
        util::LockAtomic lockAtomic( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _mGroups );

        auto it = _groups.find( groupName );
        if( it == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];

        for( auto itChannel = group->channels.begin(); itChannel != group->channels.end(); itChannel++ ) {
            delete itChannel->second->messages;
            delete itChannel->second;
        }

        delete _groups[groupName];
        _groups.erase( it );
    }

    void Manager::addChannel(
        const char *groupName,
        const char *channelName,
        const char *path,
        util::Types::ChannelSettings &settings
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) != group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = new Channel{};
        channel->messages = new Messages( path, settings );

        group->channels[channelName] = channel;
    }

    void Manager::updateChannelSettings(
        const char *groupName,
        const char *channelName,
        util::Types::ChannelSettings &settings
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw -1;
        }

        group->channels[channelName]->messages->updateLimits( settings );
    }

    void Manager::removeChannel(
        const char *groupName,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        auto itChannel = group->channels.find( channelName );
        if( itChannel == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName];

        delete channel->messages;
        group->channels.erase( itChannel );
    }

    void Manager::joinConsumer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicChannels( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto itConsumer = channel->consumers.find( fd );
        if( itConsumer != channel->consumers.end() ) {
            throw util::Error::DUPLICATE_CONSUMER;
        }

        channel->consumers[fd] = std::list<unsigned int>();
    }

    unsigned int Manager::_calculateTotalConsumersByMessagedID(
        std::map<unsigned int, std::list<unsigned int>> &map,
        unsigned int msgID
    ) {
        unsigned int total = 0;

        for( auto itConsumer = map.begin(); itConsumer != map.end(); itConsumer++ ) {
            for( auto itMsg = itConsumer->second.begin(); itMsg != itConsumer->second.end(); itMsg++ ) {
                if( *itMsg == msgID ) {
                    total++;
                }
            }
        }

        return total;
    }

    void Manager::leaveConsumer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicChannels( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto itConsumer = channel->consumers.find( fd );
        if( itConsumer == channel->consumers.end() ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        for( auto itMsg = itConsumer->second.begin(); itMsg != itConsumer->second.end(); itConsumer++ ) {
            auto total = _calculateTotalConsumersByMessagedID( channel->consumers, *itMsg );

            if( total == 1 ) {
                channel->messages->free( *itMsg );
            }
        }

        channel->consumers.erase( itConsumer );
    }

    void Manager::joinProducer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicChannels( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( channel->mProducers );

        if( channel->producers.find( fd ) != channel->producers.end() ) {
            throw util::Error::DUPLICATE_PRODUCER;
        }

        channel->producers[fd] = true;
    }

    void Manager::leaveProducer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicChannels( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( channel->mProducers );

        auto itProducer = channel->producers.find( fd );
        if( itProducer == channel->producers.end() ) {
            return;
        }

        channel->producers.erase( itProducer );
    }

    bool Manager::_isConsumer( std::map<unsigned int, std::list<unsigned int>> &map, unsigned int fd ) {
        auto it = map.find( fd );

        return it != map.end();
    }

    void Manager::_checkConsumer( std::map<unsigned int, std::list<unsigned int>> &map, unsigned int fd ) {
        if( !_isConsumer( map, fd ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }
    }

    bool Manager::_isProducer( std::map<unsigned int, bool> &map, unsigned int fd ) {
        auto it = map.find( fd );

        return it != map.end();
    }

    void Manager::_checkProducer( std::map<unsigned int, bool> &map, unsigned int fd ) {
        if( !_isProducer( map, fd ) ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }
    }

    unsigned int Manager::createMessageForQ(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int length,
        Messages::WRData &wrData,
        char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        channel->uuid.generate( uuid );

        auto id = channel->messages->add( length, wrData );

        channel->messages->setUUID( id, uuid );

        return id;
    }

    unsigned int Manager::createMessageForBroadcast(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int length,
        Messages::WRData &wrData
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        return channel->messages->add( length, wrData );
    }

    unsigned int Manager::createMessageForReplication(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int length,
        Messages::WRData &wrData,
        const char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        channel->uuid.add( uuid );

        auto id = channel->messages->add( length, wrData );

        channel->messages->setUUID( id, uuid );

        return id;
    }

    void Manager::removeMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        char uuid[util::UUID::LENGTH+1]{};

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto isConsumer = _isConsumer( channel->consumers, fd );
        auto isProducer = _isProducer( channel->producers, fd );

        if( isConsumer || isProducer ) {
            if( isConsumer && _calculateTotalConsumersByMessagedID( channel->consumers, id ) != 0 ) {
                return;
            }

            channel->messages->getUUID( id, uuid );

            if( uuid[0] != 0 ) {
                channel->uuid.free( uuid );
            }

            channel->messages->free( id );
        } else {
            throw util::Error::NOT_FOUND_SESSION;
        }


    }

    void Manager::removeMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        const char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mProducers );

        _checkConsumer( channel->consumers, fd );

        auto id = channel->messages->getID( uuid );

        auto itMsg = std::find( channel->QList.begin(), channel->QList.end(), id );
        if( itMsg != channel->QList.end() ) {
            channel->QList.erase( itMsg );
        }

        channel->uuid.free( uuid );
        channel->messages->free( uuid );
    }

    void Manager::recv(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id,
        Messages::WRData &wrData
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        channel->messages->recv( id, fd, wrData );
    }

    void Manager::send(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id,
        Messages::WRData &wrData
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        _checkConsumer( channel->consumers, fd );

        channel->messages->send( id, fd, wrData );
    }

    void Manager::pushMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        char uuid[util::UUID::LENGTH+1]{};

        channel->messages->getUUID( id, uuid );

        if( uuid[0] != 0 ) {
            channel->QList.push_back( id );
        } else {
            bool isAdd = false;
            for( auto itConsumer = channel->consumers.begin(); itConsumer != channel->consumers.end(); itConsumer++ ) {
                itConsumer->second.push_back( id );
                isAdd = true;
            }

            if( !isAdd ) {
                channel->messages->free( id );
            }
        }
    }

    unsigned int Manager::popMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        _checkConsumer( channel->consumers, fd );

        auto consumers = channel->consumers;

        unsigned int id = 0;

        for( auto itConsumer = consumers.begin(); itConsumer != consumers.end(); itConsumer++ ) {
            if( itConsumer->first == fd && ! itConsumer->second.empty() ) {
                id = itConsumer->second.front();
                channel->consumers[fd].pop_front();
                return id;
            }
        }

        if( !channel->QList.empty() ) {
            id = channel->QList.front();
            channel->messages->getUUID( id, uuid );
            channel->QList.pop_front();
        }

        return id;
    }

    void Manager::revertMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        _checkConsumer( channel->consumers, fd );

        char uuid[util::UUID::LENGTH+1]{};

        channel->messages->getUUID( id, uuid );

        if( uuid[0] == 0 ) {
            return;
        }

        channel->QList.push_front( id );
    }
}

#endif
 
