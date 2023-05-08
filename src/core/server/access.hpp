#ifndef SIMQ_CORE_SERVER_ACCESS
#define SIMQ_CORE_SERVER_ACCESS

#include "../../util/lock_atomic.hpp"
#include "../../util/error.h"
#include "../../crypto/hash.hpp"
#include <atomic>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <map>
#include <list>
#include <string.h>
#include <cstring>
#include <string>

namespace simq::core::server {
    class Access {
        private:
        std::atomic_uint _countGroupsWrited {0};
        std::shared_timed_mutex _mGroups;

        struct Entity {
            std::shared_timed_mutex mSessions;
            std::atomic_uint countSessionsWrited;
            std::map<unsigned int, bool> sessions;

            unsigned char password[crypto::HASH_LENGTH];
        };

        struct Consumer: Entity {};
        struct Producer: Entity {};

        struct Channel {
            std::shared_timed_mutex mConsumers;
            std::atomic_uint countConsumersWrited;
            std::map<std::string, Consumer *> consumers;

            std::shared_timed_mutex mProducers;
            std::atomic_uint countProducersWrited;
            std::map<std::string, Producer *> producers;
        };

        struct Group: Entity {
            std::shared_timed_mutex mChannels;
            std::atomic_uint countChannelsWrited;
            std::map<std::string, Channel *> channels;
        };

        std::map<std::string, Group*> _groups;

        void _wait( std::atomic_uint &atom );
        void _checkGroupSession( const char *groupName, unsigned int fd );
        void _checkConsumerSession(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
        void _checkProducerSession(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );

        void _checkIssetGroup( const char *name );
        void _checkNoIssetGroup( const char *name );
        void _checkIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd );
        void _checkNoIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd );

        void _checkIssetChannel(
            std::map<std::string, Channel*> &map,
            const char *name
        );

        void _checkNoIssetChannel(
            std::map<std::string, Channel*> &map,
            const char *name
        );

        void _checkIssetConsumer(
            std::map<std::string, Consumer*> &map,
            const char *name
        );

        void _checkNoIssetConsumer(
            std::map<std::string, Consumer*> &map,
            const char *name
        );

        void _checkIssetProducer(
            std::map<std::string, Producer*> &map,
            const char *name
        );

        void _checkNoIssetProducer(
            std::map<std::string, Producer*> &map,
            const char *name
        );
     
        public:
        void addGroup(
            const char *groupName,
            const unsigned char password[crypto::HASH_LENGTH]
        );
        void updateGroupPassword(
            const char *groupName,
            const unsigned char newPassword[crypto::HASH_LENGTH]
        );
        void removeGroup( const char *groupName );

     
        void addChannel( const char *groupName, const char *channelName );
        void removeChannel( const char *groupName, const char *channelName );
     
        void addConsumer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char password[crypto::HASH_LENGTH]
        );
        void updateConsumerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char newPassword[crypto::HASH_LENGTH]
        );
        void removeConsumer(
            const char *groupName,
            const char *channelName,
            const char *login
        );

        void addProducer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char password[crypto::HASH_LENGTH]
        );
        void updateProducerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char newPassword[crypto::HASH_LENGTH]
        );
        void removeProducer(
            const char *groupName,
            const char *channelName,
            const char *login
        );
     
        void authGroup(
            const char *groupName,
            const unsigned char password[crypto::HASH_LENGTH],
            unsigned int fd
        );
        void authConsumer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char password[crypto::HASH_LENGTH],
            unsigned int fd
        );
        void authProducer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char password[crypto::HASH_LENGTH],
            unsigned int fd
        );

     
        void logoutGroup( const char *groupName, unsigned int fd );
        void logoutConsumer(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
        void logoutProducer(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
     
        void checkPushMessage(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
        void checkPopMessage(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );

        void checkUpdateMyGroupPassword(
            const char *groupName,
            unsigned int fd
        );

        void checkCreateChannel(
            const char *groupName,
            unsigned int fd,
            const char *channelName
        );
        void checkUpdateChannelLimitMessages(
            const char *groupName,
            unsigned int fd,
            const char *channelName
        );
        void checkRemoveChannel(
            const char *groupName,
            unsigned int fd,
            const char *channelName
        );

        void checkCreateConsumer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkUpdateMyConsumerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
        void checkUpdateConsumerPassword(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkRemoveConsumer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );

        void checkCreateProducer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkUpdateMyProducerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd
        );
        void checkUpdateProducerPassword(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkRemoveProducer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );

    };

    void Access::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void Access::addGroup(
        const char *groupName,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        util::LockAtomic lockAtomicGroup( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkNoIssetGroup( groupName );

        _groups[groupName] = new Group{};
    }

    void Access::updateGroupPassword(
        const char *groupName,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        memcpy( _groups[groupName]->password, password, crypto::HASH_LENGTH );
    }

    void Access::removeGroup( const char *groupName ) {
        util::LockAtomic lockAtomicGroup( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroups( _mGroups );

        auto it = _groups.find( groupName );
        if( it == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];


        for( auto itCh = group->channels.begin(); itCh != group->channels.end(); itCh++ ) {
            auto channel = group->channels[itCh->first];

            for( auto _it = channel->consumers.begin(); _it != channel->consumers.end(); _it++ ) {
                delete _it->second;
            }

            for( auto _it = channel->producers.begin(); _it != channel->producers.end(); _it++ ) {
                delete _it->second;
            }

            delete itCh->second;
        }

        delete group;

        _groups.erase( it );
    }

    void Access::addChannel( const char *groupName, const char *channelName ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkNoIssetChannel( group->channels, channelName );

        group->channels[channelName] = new Channel{};
    }

    void Access::removeChannel( const char *groupName, const char *channelName ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        auto itCh = group->channels.find( channelName );
        if( itCh == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName];

        for( auto it = channel->consumers.begin(); it != channel->consumers.end(); it++ ) {
            delete it->second;
        }

        for( auto it = channel->producers.begin(); it != channel->producers.end(); it++ ) {
            delete it->second;
        }

        delete group->channels[channelName];
        group->channels.erase( itCh );
    }

    void Access::addConsumer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkNoIssetConsumer( channel->consumers, login );

        auto item = new Consumer{};
        memcpy( item->password, password, crypto::HASH_LENGTH );

        channel->consumers[login] = item;
    }

    void Access::updateConsumerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login];

        util::LockAtomic lockAtomicConsumerSessions( consumer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        memcpy( channel->consumers[login]->password, password, crypto::HASH_LENGTH );
        channel->consumers[login]->sessions.clear();
    }

    void Access::removeConsumer(
        const char *groupName,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        auto itConsumer = channel->consumers.find( login );
        if( itConsumer == channel->consumers.end() ) {
            return;
        }

        delete itConsumer->second;

        channel->consumers.erase( itConsumer );
    }

    void Access::addProducer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkNoIssetProducer( channel->producers, login );

        auto item = new Producer{};
        memcpy( item->password, password, crypto::HASH_LENGTH );

        channel->producers[login] = item;
    }

    void Access::updateProducerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH]
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login];

        util::LockAtomic lockAtomicProducerSessions( producer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        memcpy( channel->producers[login]->password, password, crypto::HASH_LENGTH );

        channel->producers[login]->sessions.clear();
    }

    void Access::removeProducer(
        const char *groupName,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        auto itProducer = channel->producers.find( login );
        if( itProducer == channel->producers.end() ) {
            return;
        }

        channel->producers.erase( itProducer );
    }

    void Access::authGroup(
        const char *groupName,
        const unsigned char password[crypto::HASH_LENGTH],
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicGroupSessions( group->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        if( strncmp(
            ( const char * )group->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        _checkNoIssetSessions( group->sessions, fd );

        group->sessions[fd] = true;
    }

    void Access::authConsumer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH],
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login];

        util::LockAtomic lockAtomicConsumerSessions( consumer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        _checkNoIssetSessions( consumer->sessions, fd );

        consumer->sessions[fd] = true;
    }

    void Access::authProducer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char password[crypto::HASH_LENGTH],
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login];

        util::LockAtomic lockAtomicProducerSessions( producer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        _checkNoIssetSessions( producer->sessions, fd );

        producer->sessions[fd] = true;
    }

    void Access::logoutGroup( const char *groupName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];

        util::LockAtomic lockAtomicGroupSessions( group->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        auto it = group->sessions.find( fd );

        if( it == group->sessions.end() ) {
            return;
        }

        group->sessions.erase( it );
    }

    void Access::logoutConsumer(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        if( channel->consumers.find( login ) == channel->consumers.end() ) {
            return;
        }

        auto consumer = channel->consumers[login];

        util::LockAtomic lockAtomicConsumerSessions( consumer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        auto it = consumer->sessions.find( fd );

        if( it == consumer->sessions.end() ) {
            return;
        }

        consumer->sessions.erase( it );
    }

    void Access::logoutProducer(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        if( channel->producers.find( login ) == channel->producers.end() ) {
            return;
        }

        auto producer = channel->producers[login];

        util::LockAtomic lockAtomicProducerSessions( producer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        auto it = producer->sessions.find( fd );

        if( it == producer->sessions.end() ) {
            return;
        }

        producer->sessions.erase( it );
    }

    void Access::_checkGroupSession( const char *groupName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );
    }

    void Access::_checkConsumerSession(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login];

        _wait( consumer->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        _checkIssetSessions( consumer->sessions, fd );
    }

    void Access::_checkProducerSession(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mConsumers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login];

        _wait( producer->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        _checkIssetSessions( producer->sessions, fd );
    }

    void Access::checkPushMessage(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _checkProducerSession( groupName, channelName, login, fd );
    }

    void Access::checkPopMessage(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _checkConsumerSession( groupName, channelName, login, fd );
    }

    void Access::checkUpdateMyGroupPassword(
        const char *groupName,
        unsigned int fd
    ) {
        _checkGroupSession( groupName, fd );
    }

    void Access::checkUpdateMyConsumerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _checkConsumerSession( groupName, channelName, login, fd );
    }

    void Access::checkUpdateMyProducerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd
    ) {
        _checkProducerSession( groupName, channelName, login, fd );
    }

    void Access::_checkIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd ) {
        if( map.find( fd ) == map.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }
    }

    void Access::_checkNoIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd ) {
        if( map.find( fd ) != map.end() ) {
            throw util::Error::DUPLICATE_SESSION;
        }
    }

    void Access::_checkIssetGroup( const char *name ) {
        if( _groups.find( name ) == _groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }
    }

    void Access::_checkNoIssetGroup( const char *name ) {
        if( _groups.find( name ) != _groups.end() ) {
            throw util::Error::DUPLICATE_GROUP;
        }
    }

    void Access::_checkIssetChannel(
        std::map<std::string, Channel*> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }
    }

    void Access::_checkNoIssetChannel(
        std::map<std::string, Channel*> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }
    }

    void Access::_checkIssetConsumer(
        std::map<std::string, Consumer*> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }
    }

    void Access::_checkNoIssetConsumer(
        std::map<std::string, Consumer*> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_CONSUMER;
        }
    }

    void Access::_checkIssetProducer(
        std::map<std::string, Producer*> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }
    }

    void Access::_checkNoIssetProducer(
        std::map<std::string, Producer*> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_PRODUCER;
        }
    }

    void Access::checkCreateChannel(
        const char *groupName,
        unsigned int fd,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkNoIssetChannel( group->channels, channelName );
    }

    void Access::checkUpdateChannelLimitMessages(
        const char *groupName,
        unsigned int fd,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
    }

    void Access::checkRemoveChannel(
        const char *groupName,
        unsigned int fd,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
    }

    void Access::checkCreateConsumer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];
        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkNoIssetConsumer( channel->consumers, login );
    }

    void Access::checkUpdateConsumerPassword(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
        auto channel = group->channels[channelName];
        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );
        _checkIssetConsumer( channel->consumers, login );
    }

    void Access::checkRemoveConsumer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];
        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );
    }

    void Access::checkCreateProducer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];
        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkNoIssetProducer( channel->producers, login );
    }

    void Access::checkUpdateProducerPassword(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];
        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );
    }

    void Access::checkRemoveProducer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName];

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName];
        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );
    }

}

#endif
