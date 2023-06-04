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
#include <memory>
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
            std::map<std::string, std::unique_ptr<Consumer>> consumers;

            std::shared_timed_mutex mProducers;
            std::atomic_uint countProducersWrited;
            std::map<std::string, std::unique_ptr<Producer>> producers;
        };

        struct Group: Entity {
            std::shared_timed_mutex mChannels;
            std::atomic_uint countChannelsWrited;
            std::map<std::string, std::unique_ptr<Channel>> channels;
        };

        std::map<std::string, std::unique_ptr<Group>> _groups;

        void _wait( std::atomic_uint &atom );
        void _checkGroupSession(
            const char *groupName,
            unsigned int fd,
            const unsigned char *password = nullptr
        );
        void _checkConsumerSession(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd,
            const unsigned char *password = nullptr
        );
        void _checkProducerSession(
            const char *groupName,
            const char *channelName,
            const char *login,
            unsigned int fd,
            const unsigned char *password = nullptr
        );

        void _checkIssetGroup( const char *name );
        void _checkNoIssetGroup( const char *name );
        void _checkIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd );
        void _checkNoIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd );

        void _checkIssetChannel(
            std::map<std::string, std::unique_ptr<Channel>> &map,
            const char *name
        );

        void _checkNoIssetChannel(
            std::map<std::string, std::unique_ptr<Channel>> &map,
            const char *name
        );

        void _checkIssetConsumer(
            std::map<std::string, std::unique_ptr<Consumer>> &map,
            const char *name
        );

        void _checkNoIssetConsumer(
            std::map<std::string, std::unique_ptr<Consumer>> &map,
            const char *name
        );

        void _checkIssetProducer(
            std::map<std::string, std::unique_ptr<Producer>> &map,
            const char *name
        );

        void _checkNoIssetProducer(
            std::map<std::string, std::unique_ptr<Producer>> &map,
            const char *name
        );
     
        public:
        void addGroup(
            const char *groupName,
            const unsigned char *password
        );
        void updateGroupPassword(
            const char *groupName,
            const unsigned char *newPassword
        );
        void removeGroup( const char *groupName );

     
        void addChannel( const char *groupName, const char *channelName );
        void removeChannel( const char *groupName, const char *channelName );
     
        void addConsumer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *password
        );
        void updateConsumerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *newPassword
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
            const unsigned char *password
        );
        void updateProducerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *newPassword
        );
        void removeProducer(
            const char *groupName,
            const char *channelName,
            const char *login
        );
     
        void authGroup(
            const char *groupName,
            const unsigned char *password,
            unsigned int fd
        );
        void authConsumer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *password,
            unsigned int fd
        );
        void authProducer(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *password,
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
            const unsigned char *password,
            unsigned int fd
        );

        void checkToGroup(
            const char *groupName,
            unsigned int fd
        );

        void checkToChannel(
            const char *groupName,
            const char *channelName,
            unsigned int fd
        );

        void checkAddChannel(
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

        void checkAddConsumer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkUpdateMyConsumerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *password,
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

        void checkAddProducer(
            const char *groupName,
            unsigned int fd,
            const char *channelName,
            const char *login
        );
        void checkUpdateMyProducerPassword(
            const char *groupName,
            const char *channelName,
            const char *login,
            const unsigned char *password,
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
        const unsigned char *password
    ) {
        util::LockAtomic lockAtomicGroup( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkNoIssetGroup( groupName );

        _groups[groupName] = std::make_unique<Group>();
        memcpy( _groups[groupName]->password, password, crypto::HASH_LENGTH );
    }

    void Access::updateGroupPassword(
        const char *groupName,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );
        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicGroupSessions( group->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        memcpy( group->password, password, crypto::HASH_LENGTH );

        group->sessions.clear();
    }

    void Access::removeGroup( const char *groupName ) {
        util::LockAtomic lockAtomicGroup( _countGroupsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroups( _mGroups );

        _groups.erase( groupName );
    }

    void Access::addChannel( const char *groupName, const char *channelName ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkNoIssetChannel( group->channels, channelName );

        group->channels[channelName] = std::make_unique<Channel>();
    }

    void Access::removeChannel( const char *groupName, const char *channelName ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicChannels( group->countChannelsWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannels( group->mChannels );

        group->channels.erase( channelName );
    }

    void Access::addConsumer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkNoIssetConsumer( channel->consumers, login );

        channel->consumers[login] = std::make_unique<Consumer>();
        memcpy( channel->consumers[login]->password, password, crypto::HASH_LENGTH );
    }

    void Access::updateConsumerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login].get();

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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicConsumers( channel->countConsumersWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        channel->consumers.erase( login );
    }

    void Access::addProducer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkNoIssetProducer( channel->producers, login );

        channel->producers[login] = std::make_unique<Producer>();
        memcpy( channel->producers[login]->password, password, crypto::HASH_LENGTH );
    }

    void Access::updateProducerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login].get();

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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        util::LockAtomic lockAtomicProducers( channel->countProducersWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducers( channel->mProducers );

        channel->producers.erase( login );
    }

    void Access::authGroup(
        const char *groupName,
        const unsigned char *password,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

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
        const unsigned char *password,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login].get();

        util::LockAtomic lockAtomicConsumerSessions( consumer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        if( strncmp(
            ( const char * )consumer->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        _checkNoIssetSessions( consumer->sessions, fd );

        consumer->sessions[fd] = true;
    }

    void Access::authProducer(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login].get();

        util::LockAtomic lockAtomicProducerSessions( producer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        if( strncmp(
            ( const char * )producer->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        _checkNoIssetSessions( producer->sessions, fd );

        producer->sessions[fd] = true;
    }

    void Access::logoutGroup( const char *groupName, unsigned int fd ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        if( _groups.find( groupName ) == _groups.end() ) {
            return;
        }

        auto group = _groups[groupName].get();

        util::LockAtomic lockAtomicGroupSessions( group->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        group->sessions.erase( fd );
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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        if( channel->consumers.find( login ) == channel->consumers.end() ) {
            return;
        }

        auto consumer = channel->consumers[login].get();

        util::LockAtomic lockAtomicConsumerSessions( consumer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        consumer->sessions.erase( fd );
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

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        if( group->channels.find( channelName ) == group->channels.end() ) {
            return;
        }

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        if( channel->producers.find( login ) == channel->producers.end() ) {
            return;
        }

        auto producer = channel->producers[login].get();

        util::LockAtomic lockAtomicProducerSessions( producer->countSessionsWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        producer->sessions.erase( fd );
    }

    void Access::_checkGroupSession(
        const char *groupName,
        unsigned int fd,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        if( password != nullptr && strncmp(
            ( const char * )group->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }
    }

    void Access::_checkConsumerSession(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );

        auto consumer = channel->consumers[login].get();

        _wait( consumer->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumerSessions( consumer->mSessions );

        _checkIssetSessions( consumer->sessions, fd );

        if( password != nullptr && strncmp(
            ( const char * )consumer->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }
    }

    void Access::_checkProducerSession(
        const char *groupName,
        const char *channelName,
        const char *login,
        unsigned int fd,
        const unsigned char *password
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();

        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mConsumers );

        _checkIssetProducer( channel->producers, login );

        auto producer = channel->producers[login].get();

        _wait( producer->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducerSessions( producer->mSessions );

        _checkIssetSessions( producer->sessions, fd );

        if( password != nullptr && strncmp(
            ( const char * )producer->password,
            (const char *)password,
            crypto::HASH_LENGTH
        ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }
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
        const unsigned char *password,
        unsigned int fd
    ) {
        _checkGroupSession( groupName, fd, password );
    }

    void Access::checkUpdateMyConsumerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password,
        unsigned int fd
    ) {
        _checkConsumerSession( groupName, channelName, login, fd, password );
    }

    void Access::checkUpdateMyProducerPassword(
        const char *groupName,
        const char *channelName,
        const char *login,
        const unsigned char *password,
        unsigned int fd
    ) {
        _checkProducerSession( groupName, channelName, login, fd, password );
    }

    void Access::_checkIssetSessions( std::map<unsigned int, bool> &map, unsigned int fd ) {
        if( map.find( fd ) == map.end() ) {
            throw util::Error::ACCESS_DENY;
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
        std::map<std::string, std::unique_ptr<Channel>> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }
    }

    void Access::_checkNoIssetChannel(
        std::map<std::string, std::unique_ptr<Channel>> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }
    }

    void Access::_checkIssetConsumer(
        std::map<std::string, std::unique_ptr<Consumer>> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }
    }

    void Access::_checkNoIssetConsumer(
        std::map<std::string, std::unique_ptr<Consumer>> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_CONSUMER;
        }
    }

    void Access::_checkIssetProducer(
        std::map<std::string, std::unique_ptr<Producer>> &map,
        const char *name
    ) {
        if( map.find( name ) == map.end() ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }
    }

    void Access::_checkNoIssetProducer(
        std::map<std::string, std::unique_ptr<Producer>> &map,
        const char *name
    ) {
        if( map.find( name ) != map.end() ) {
            throw util::Error::DUPLICATE_PRODUCER;
        }
    }

    void Access::checkToChannel(
        const char *groupName,
        const char *channelName,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
    }

    void Access::checkToGroup(
        const char *groupName,
        unsigned int fd
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );
    }

    void Access::checkAddChannel(
        const char *groupName,
        unsigned int fd,
        const char *channelName
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

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

        auto group = _groups[groupName].get();

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

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
    }

    void Access::checkAddConsumer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();
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

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );
        auto channel = group->channels[channelName].get();
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

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();
        _wait( channel->countConsumersWrited );
        std::shared_lock<std::shared_timed_mutex> lockConsumers( channel->mConsumers );

        _checkIssetConsumer( channel->consumers, login );
    }

    void Access::checkAddProducer(
        const char *groupName,
        unsigned int fd,
        const char *channelName,
        const char *login
    ) {
        _wait( _countGroupsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroups( _mGroups );

        _checkIssetGroup( groupName );

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();
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

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();
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

        auto group = _groups[groupName].get();

        _wait( group->countSessionsWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroupSessions( group->mSessions );

        _checkIssetSessions( group->sessions, fd );

        _wait( group->countChannelsWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannels( group->mChannels );

        _checkIssetChannel( group->channels, channelName );

        auto channel = group->channels[channelName].get();
        _wait( channel->countProducersWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducers( channel->mProducers );

        _checkIssetProducer( channel->producers, login );
    }

}

#endif
