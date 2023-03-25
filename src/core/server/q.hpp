#ifndef SIMQ_CORE_SERVER_Q
#define SIMQ_CORE_SERVER_Q

#include <shared_mutex>
#include <map>
#include <string>
#include <deque>
#include <atomic>
#include <iterator>
#include "../../util/lock_atomic.hpp"
#include "../../util/error.h"
#include "../../util/buffer.hpp"

namespace simq::core::server {
    class Q {
        public:
            struct ChannelSettings {
                unsigned int minMessageSize;
                unsigned int maxMessageSize;
                unsigned int maxMessagesInMemory;
                unsigned int maxMessagesOnDisk;
            };

            struct WRData {
                bool isPart;
                bool isFull;
            };

        private:
            struct MessageData {
                unsigned int length;
                unsigned int wrLength;
                unsigned int offset;
            };

            const unsigned int SIZE_ITEM_PACKET = 10'000;

            struct Channel {
                util::Buffer *buffer;
                std::mutex *m;
                ChannelSettings settings;
                unsigned int countInMemory;
                unsigned int countOnDisk;
                MessageData **messages;
                unsigned int lengthMessages;
                std::deque<unsigned int> q;
                std::map<unsigned int, std::queue<unsigned int>>subscribers;
                std::map<unsigned int, unsigned int> publicMessagesCounter;
            };

            std::atomic_uint countGroupWrited {0};
            std::atomic_uint countChannelWrited {0};
            std::shared_timed_mutex mGroup;
            std::shared_timed_mutex mChannel;

            std::map<std::string, std::map<std::string, Channel>> groups;

            void _wait( std::atomic_uint &atom );
        public:
            void addGroup( const char *group );
            void addChannel( const char *group, const char *channel, const char *path, ChannelSettings &settings );
            void updateChannelSettings( const char *group, const char *channel, ChannelSettings &settings );
            void removeGroup( const char *group );
            void removeChannel( const char *group, const char *channel );

            unsigned int createMessage( const char *group, const char *channel, unsigned int length );
            void removeMessage( const char *group, const char *channel, unsigned int id );

            void recv(
                const char *group,
                const char *channel,
                unsigned int fd,
                unsigned int id,
                WRData &data
            );
            void send(
                const char *group,
                const char *channel,
                unsigned int fd,
                unsigned int id,
                WRData &data
            );

            void revertMessageToQ( const char *group, const char *channel, unsigned int id );
            void pushMessageToQ( const char *group, const char *channel, unsigned int id );
            unsigned int popMessageFromQ( const char *group, const char *channel );

            void subscribeToChannel( const char *group, const char *channel, unsigned int fd );
            void unsubscribeFromChannel( const char *group, const char *channel, unsigned int fd );

            void publishMessage( const char *group, const char *channel, unsigned int id );
    };

    void Q::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void Q::addGroup( const char *group ) {
        util::LockAtomic lockAtomicGroup( countGroupWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        if( groups.count( group ) ) {
            throw util::Error::DUPLICATE_GROUP;
        }

        std::map<std::string, Channel> channels;
        groups[group] = channels;
    }

    void Q::removeGroup( const char *group ) {
        util::LockAtomic lockAtomicGroup( countGroupWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        }
    }

    void Q::addChannel( const char *group, const char *channel, const char *path, ChannelSettings &settings ) {
        _wait( countGroupWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( groups[group].count( channel ) ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        Channel _channel{0};
        _channel.buffer = new util::Buffer( path );
        _channel.m = new std::mutex;
        _channel.settings = settings;
        groups[group][channel] = _channel;
    }

    void Q::updateChannelSettings( const char *group, const char *channel, ChannelSettings &settings ) {
        _wait( countGroupWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        groups[group][channel].settings = settings;
    }

    void Q::removeChannel( const char *group, const char *channel ) {
        _wait( countGroupWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            return;
        }

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        auto iter = groups[group].find( channel );
        delete iter->second.buffer;
        delete iter->second.m;
        groups[group].erase( iter );
    }

    unsigned int Q::createMessage( const char *group, const char *channel, unsigned int length ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &settings = iter->second.settings;
        auto &data = iter->second;

        if( settings.minMessageSize > length || settings.maxMessageSize < length ) {
            throw util::Error::WRONG_MESSAGE_SIZE;
        } else if( data.countOnDisk == settings.maxMessagesOnDisk && data.countInMemory == settings.maxMessagesInMemory ) {
            throw util::Error::EXCEED_LIMIT;
        }

        bool inMemory = false;
        bool onDisk = false;
        {
            std::lock_guard<std::mutex> lockItem( *groups[group][channel].m );
            if( data.countInMemory == settings.maxMessagesInMemory ) {
                groups[group][channel].countOnDisk++;
                onDisk = true;
            } else {
                groups[group][channel].countInMemory++;
                inMemory = true;
            }
        }

        if( inMemory ) {
            return groups[group][channel].buffer->allocate( length < 4096 ? length : 4096 );
        }

        return groups[group][channel].buffer->allocateOnDisk();
    }
}

#endif
 
