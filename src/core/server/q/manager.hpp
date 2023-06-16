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
#include <memory>
#include "../../../util/lock_atomic.hpp"
#include "../../../util/error.h"
#include "../../../util/types.h"
#include "../../../util/uuid.hpp"
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

                std::unique_ptr<Messages> messages;

                std::shared_timed_mutex mQList;
                std::atomic_uint countQListWrited;
                std::list<unsigned int> QList;
                std::map<unsigned int, unsigned int> signals;
            };

            struct Group {
                std::shared_timed_mutex mChannels;
                std::atomic_uint countChannelsWrited;
                std::map<std::string, std::unique_ptr<Channel>> channels;
            };

            std::shared_timed_mutex _mGroups;
            std::atomic_uint _countGroupsWrited{0};

            std::map<std::string, std::unique_ptr<Group>> _groups;

            void _wait( std::atomic_uint &atom );

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
                util::types::ChannelLimitMessages &limitMessages
            );
            void updateChannelLimitMessages(
                const char *groupName,
                const char *channelName,
                util::types::ChannelLimitMessages &limitMessages
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
                char *uuid
            );

            unsigned int createMessageForBroadcast(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int length
            );

            unsigned int createMessageForReplication(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int length,
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

            unsigned int recv(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id
            );
            unsigned int send(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id,
                unsigned int offset
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
                unsigned int &length,
                char *uuid
            );
            void revertMessage(
                const char *groupName,
                const char *channelName,
                unsigned int fd,
                unsigned int id
            );

            void clearQ (
                const char *groupName,
                const char *channelName
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

        _groups[groupName] = std::make_unique<Group>();
    }

    void Manager::removeGroup( const char *groupName ) {
        util::LockAtomic lockAtomic( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _mGroups );

        _groups.erase( groupName );
    }

    void Manager::addChannel(
        const char *groupName,
        const char *channelName,
        const char *path,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) != group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        group->channels[channelName] = std::make_unique<Channel>();
        group->channels[channelName]->messages = std::make_unique<Messages>( path, limitMessages );
    }

    void Manager::updateChannelLimitMessages(
        const char *groupName,
        const char *channelName,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        group->channels[channelName]->messages->updateLimits( limitMessages );
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

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        group->channels.erase( channelName );
    }

    void Manager::joinConsumer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicChannels( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto itConsumer = channel->consumers.find( fd );
        if( itConsumer != channel->consumers.end() ) {
            throw util::Error::DUPLICATE_CONSUMER;
        }

        channel->consumers[fd] = std::list<unsigned int>();
    }

    void Manager::leaveConsumer( const char *groupName, const char *channelName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicChannels( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto itConsumer = channel->consumers.find( fd );
        if( itConsumer == channel->consumers.end() ) {
            return;
        }

        for( auto itMsg = channel->consumers[fd].begin(); itMsg != channel->consumers[fd].end(); itMsg++ ) {
            auto idMsg = *itMsg;
            if( channel->signals.find( idMsg ) == channel->signals.end() ) {
                continue;
            }

            channel->signals[idMsg]--;
            if( channel->signals[idMsg] == 0 ) {
                channel->messages->free( idMsg );
                channel->signals.erase( idMsg );
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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

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
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicChannels( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( channel->mProducers );

        channel->producers.erase( fd );
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
        char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        auto id = channel->messages->addForQ( length, uuid );

        return id;
    }

    unsigned int Manager::createMessageForBroadcast(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int length
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        return channel->messages->addForBroadcast( length );
    }

    unsigned int Manager::createMessageForReplication(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int length,
        const char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        auto id = channel->messages->addForReplication( length, uuid );

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
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        auto isConsumer = _isConsumer( channel->consumers, fd );
        auto isProducer = _isProducer( channel->producers, fd );

        if( !isConsumer && !isProducer ) {
            return;
        }

        if( isConsumer && channel->signals.find( id ) != channel->signals.end() ) {
            channel->signals[id]--;

            if( channel->signals[id] != 0 ) {
                return;
            }
        }


        channel->messages->free( id );
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
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mProducers );

        if( !_isConsumer( channel->consumers, fd ) ) {
            return;
        }

        auto id = channel->messages->getID( uuid );

        auto itMsg = std::find( channel->QList.begin(), channel->QList.end(), id );
        if( itMsg != channel->QList.end() ) {
            channel->QList.erase( itMsg );
            channel->messages->free( uuid );
        }
    }

    unsigned int Manager::recv(
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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( channel->mProducers );

        _checkProducer( channel->producers, fd );

        return channel->messages->recv( id, fd );
    }

    unsigned int Manager::send(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int id,
        unsigned int offset
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        _checkConsumer( channel->consumers, fd );

        return channel->messages->send( id, fd, offset );
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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

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
                channel->consumers[itConsumer->first].push_back( id );
                isAdd = true;
            }

            if( !isAdd ) {
                channel->messages->free( id );
            } else {
                channel->signals[id] = channel->consumers.size();
            }
        }
    }

    unsigned int Manager::popMessage(
        const char *groupName,
        const char *channelName,
        unsigned int fd,
        unsigned int &length,
        char *uuid
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        _checkConsumer( channel->consumers, fd );

        auto consumers = channel->consumers;

        unsigned int id = 0;

        if( !consumers[fd].empty() ) {
            id = consumers[fd].front();
            channel->consumers[fd].pop_front();
            length = channel->messages->getLength( id );
            return id;
        }

        if( !channel->QList.empty() ) {
            id = channel->QList.front();
            channel->messages->getUUID( id, uuid );
            channel->QList.pop_front();
            length = channel->messages->getLength( id );
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
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumer( channel->mConsumers );

        if( !_isConsumer( channel->consumers, fd ) ) {
            return;
        }

        char uuid[util::UUID::LENGTH+1]{};

        channel->messages->getUUID( id, uuid );

        if( uuid[0] == 0 ) {
            return;
        }

        channel->QList.push_front( id );
    }

    void Manager::clearQ(
        const char *groupName,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicQ( channel->countQListWrited );
        std::lock_guard<std::shared_timed_mutex> lockQ( channel->mQList );

        for( auto it = channel->consumers.begin(); it != channel->consumers.end(); it++ ) {
            channel->consumers[it->first].clear();
        }

        channel->messages->clearQ();
        channel->QList.clear();
        channel->signals.clear();
    }
}

#endif
 
