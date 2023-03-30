#ifndef SIMQ_CORE_SERVER_Q
#define SIMQ_CORE_SERVER_Q

#include <mutex>
#include <shared_mutex>
#include <map>
#include <string>
#include <string.h>
#include <list>
#include <atomic>
#include <iterator>
#include "../../util/lock_atomic.hpp"
#include "../../util/error.h"
#include "../../util/types.h"
#include "../../util/buffer.hpp"

namespace simq::core::server {
    class Q {
        public:
            struct WRData {
                bool isPart;
                bool isFull;
            };

        private:
            struct MessageData {
                unsigned int length;
                unsigned int wrLength;
                unsigned int offset;
                bool isMemory;
            };

            struct Session {
                char *group;
                char *channel;
            };

            std::map<unsigned int, Session> sessions;
            std::shared_timed_mutex mSess;
            std::atomic_uint countSessWrited {0};

            const unsigned int MESSAGES_IN_PACKET = 10'000;

            struct Channel {
                util::Buffer *buffer;
                std::mutex *mMessages;
                std::mutex *mQ;
                util::Types::ChannelSettings settings;
                unsigned int countInMemory;
                unsigned int countOnDisk;
                MessageData ***messages;
                unsigned int lengthMessages;
                std::list<unsigned int> q;
                /*
                std::mutex *mPublish;
                std::map<unsigned int, std::queue<unsigned int>>subscribers;
                std::map<unsigned int, unsigned int> publicMessagesCounter;
                */
            };

            std::atomic_uint countGroupWrited {0};
            std::atomic_uint countChannelWrited {0};
            std::shared_timed_mutex mGroup;
            std::shared_timed_mutex mChannel;

            std::map<std::string, std::map<std::string, Channel>> groups;

            void _wait( std::atomic_uint &atom );
            void _expandMessagesPacket( const char *group, const char *channel );
            void _addMessage( const char *group, const char *channel, unsigned int id, unsigned int length, bool isMemory );
        public:
            void addGroup( const char *group );
            void addChannel(
                const char *group,
                const char *channel,
                const char *path,
                util::Types::ChannelSettings &settings
            );
            void updateChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings &settings
            );
            void removeGroup( const char *group );
            void removeChannel( const char *group, const char *channel );

            void auth( const char *group, const char *channel, unsigned int fd );
            void logout( unsigned int fd );

            unsigned int createMessage( const char *group, const char *channel, unsigned int length );
            void removeMessage( const char *group, const char *channel, unsigned int id );

            void recv(
                const char *group,
                const char *channel,
                unsigned int fd,
                unsigned int id,
                WRData &wrData
            );
            void send(
                const char *group,
                const char *channel,
                unsigned int fd,
                unsigned int id,
                WRData &wrData
            );

            void pushMessageToQ( const char *group, const char *channel, unsigned int id );
            unsigned int popMessageFromQ( const char *group, const char *channel );
            void revertMessageToQ( const char *group, const char *channel, unsigned int id );

            //void subscribeToChannel( const char *group, const char *channel, unsigned int fd );
            //void unsubscribeFromChannel( const char *group, const char *channel, unsigned int fd );

            //void publishMessage( const char *group, const char *channel, unsigned int id );
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

        // TODO: create remove channels

    }

    void Q::addChannel(
        const char *group,
        const char *channel,
        const char *path,
        util::Types::ChannelSettings &settings
    ) {
        _wait( countGroupWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( groups[group].count( channel ) ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }

        // TODO validate settings

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        Channel _channel{0};
        _channel.buffer = new util::Buffer( path );
        _channel.mMessages = new std::mutex;
        _channel.mQ = new std::mutex;
        _channel.settings = settings;
        _channel.messages = new MessageData**[1]{};
        _channel.messages[0] = new MessageData*[MESSAGES_IN_PACKET]{};
        groups[group][channel] = _channel;
    }

    void Q::updateChannelSettings(
        const char *group,
        const char *channel,
        util::Types::ChannelSettings &settings
    ) {
        _wait( countGroupWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        // TODO validate settings

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
        delete iter->second.mMessages;
        delete iter->second.mQ;
        auto lengthMessages = iter->second.lengthMessages / MESSAGES_IN_PACKET;
        if( iter->second.lengthMessages - lengthMessages * MESSAGES_IN_PACKET ) {
            lengthMessages++;
        }
        for( int i = 0; i < lengthMessages; i++ ) {
            for( int c = 0; c < MESSAGES_IN_PACKET; c++ ) {
                if( iter->second.messages[i][c] ) {
                    delete iter->second.messages[i][c];
                }
            }
            delete[] iter->second.messages[i];
        }

        delete iter->second.messages;
        groups[group].erase( iter );
    }

    void Q::_expandMessagesPacket( const char *group, const char *channel ) {
        auto &data = groups[group].find( channel )->second;

        if( data.lengthMessages % MESSAGES_IN_PACKET != 0 ) {
            return;
        }

        auto length = data.lengthMessages / MESSAGES_IN_PACKET;

        auto tmpBuffer = new MessageData**[length+1]{};

        if( length != 0 ) {
            memcpy( tmpBuffer, data.messages, length * sizeof( MessageData ** ) );
            delete[] data.messages;
        }
        data.messages[length] = new MessageData*[MESSAGES_IN_PACKET]{};
    }

    void Q::_addMessage( const char *group, const char *channel, unsigned int id, unsigned int length, bool isMemory ) {
        auto offsetPacket = id / MESSAGES_IN_PACKET;
        auto offset = groups[group][channel].lengthMessages - offsetPacket * MESSAGES_IN_PACKET;;

        auto data = new MessageData{};
        data->length = length;
        data->isMemory = isMemory;

        groups[group][channel].messages[offsetPacket][offset] = data;
    }

    void Q::auth( const char *group, const char *channel, unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );
        
        if( sessions.count( fd ) ) {
            throw util::Error::DUPLICATE_SESSION;
        }
        
        _wait( countGroupWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroup( mGroup );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        _wait( countChannelWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannel( mChannel );

        if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        Session sess;
        auto lGroup = strlen( group );
        auto lChannel = strlen( group );
        sess.group = new char[lGroup+1]{};
        sess.channel = new char[lChannel+1]{};

        sessions[fd] = sess;
    }

    void Q::logout( unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );
        
        auto iter = sessions.find( fd );

        if( iter == sessions.end() ) {
            return;
        }

        delete[] iter->second.group;
        delete[] iter->second.channel;

        sessions.erase( iter );
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

        std::lock_guard<std::mutex> lockItem( *groups[group][channel].mMessages );

        unsigned int messageId;
        bool isMemory = false;

        if( data.countInMemory == settings.maxMessagesInMemory ) {
            groups[group][channel].countOnDisk++;
            messageId = groups[group][channel].buffer->allocateOnDisk();
        } else {
            isMemory = true;
            groups[group][channel].countInMemory++;
            messageId = groups[group][channel].buffer->allocate(
                length < 4096 ? length : 4096
            );
        }

        if( messageId > groups[group][channel].lengthMessages ) {
            groups[group][channel].lengthMessages = messageId;
            _expandMessagesPacket( group, channel );
        }

        _addMessage( group, channel, messageId, length, isMemory );

        return messageId;
    }

    void Q::removeMessage( const char *group, const char *channel, unsigned int id ) {
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

        std::lock_guard<std::mutex> lockItem( *groups[group][channel].mMessages );

        if( id > data.lengthMessages ) {
            throw util::Error::WRONG_PARAM;
        }

        auto offsetPacket = id / MESSAGES_IN_PACKET;
        auto offset = data.lengthMessages - offsetPacket * MESSAGES_IN_PACKET;;

        if( data.messages[offsetPacket][offset]->isMemory ) {
            data.countInMemory--;
        } else {
            data.countOnDisk--;
        }
        delete data.messages[offsetPacket][offset];
        data.messages[offsetPacket][offset] = nullptr;
    }

    void Q::recv(
        const char *group,
        const char *channel,
        unsigned int fd,
        unsigned int id,
        WRData &wrData
    ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &data = iter->second;

        if( id > data.lengthMessages ) {
            throw util::Error::WRONG_PARAM;
        }

        // TODO recv
        //data.buffer->recv( id, fd, 0 );
    }

    void Q::send(
        const char *group,
        const char *channel,
        unsigned int fd,
        unsigned int id,
        WRData &wrData
    ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &data = iter->second;

        if( id > data.lengthMessages ) {
            throw util::Error::WRONG_PARAM;
        }

        // TODO send
        //data.buffer->recv( id, fd, 0 );
    }

    void Q::pushMessageToQ( const char *group, const char *channel, unsigned int id ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &data = iter->second;

        if( id > data.lengthMessages ) {
            throw util::Error::WRONG_PARAM;
        }

        std::lock_guard<std::mutex> lockQ( *data.mQ );
        data.q.push_back( id );
    }

    unsigned int Q::popMessageFromQ( const char *group, const char *channel ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &data = iter->second;

        std::lock_guard<std::mutex> lockQ( *data.mQ );
        auto messageId = data.q.front();
        data.q.pop_front();

        return messageId;
    }

    void Q::revertMessageToQ( const char *group, const char *channel, unsigned int id ) {
        _wait( countGroupWrited );
        _wait( countChannelWrited );

        if( !groups.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !groups[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto iter = groups[group].find( channel );
        auto &data = iter->second;

        if( id > data.lengthMessages ) {
            throw util::Error::WRONG_PARAM;
        }

        std::lock_guard<std::mutex> lockQ( *data.mQ );
        data.q.push_front( id );
    }
}

#endif
 
