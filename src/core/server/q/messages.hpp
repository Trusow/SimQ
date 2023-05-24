#ifndef SIMQ_CORE_SERVER_Q_MESSAGES
#define SIMQ_CORE_SERVER_Q_MESSAGES

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <memory>
#include <string.h>
#include <vector>
#include "../../../util/uuid.hpp"
#include "../../../util/lock_atomic.hpp"
#include "buffer.hpp"
#include "../../../util/types.h"
#include "../../../util/error.h"
#include "../../../util/constants.h"

namespace simq::core::server::q {
    class Messages {
        private:
            const unsigned int MESSAGES_IN_PACKET = 10'000;
            const unsigned int MESSAGE_PACKET_SIZE = util::constants::MESSAGE_PACKET_SIZE;
            std::unique_ptr<Buffer> _buffer;

            struct Message {
                char uuid[util::UUID::LENGTH+1];
                bool isMemory;
            };

            std::shared_timed_mutex _mUUID;
            std::atomic_uint _countUUIDWrited{0};

            std::unordered_map<std::string, unsigned int> _uuid;


            std::shared_timed_mutex _m;
            std::atomic_uint _countWrited{0};

            unsigned int _totalInMemory = 0;
            unsigned int _totalOnDisk = 0;
            util::types::ChannelLimitMessages _limits;

            std::vector<std::unique_ptr<Message>> _messages;

            void _wait( std::atomic_uint &atom );
            unsigned int _allocateMessage( unsigned int length, bool &isMemory );
            void _expandMessages();
            void _validateAdd( unsigned int length );
        public:
            Messages( const char *path, util::types::ChannelLimitMessages &limits );

            void updateLimits( util::types::ChannelLimitMessages &limits );

            unsigned int addForQ( unsigned int length, char *uuid );
            unsigned int addForReplication( unsigned int length, const char *uuid );
            unsigned int addForBroadcast( unsigned int length );
            void free( unsigned int id );
            void free( const char *uuid );
            void getUUID( unsigned int id, char *uuid );
            unsigned int getID( const char *uuid );

            unsigned int recv( unsigned int id, unsigned int fd );
            unsigned int send( unsigned int id, unsigned int fd );
            void resetSend( unsigned int id );
    };

    Messages::Messages( const char *path, util::types::ChannelLimitMessages &limits ) {
        _buffer = std::make_unique<Buffer>( path );
        _messages.resize( MESSAGES_IN_PACKET );
        _limits = limits;
    }

    void Messages::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void Messages::updateLimits( util::types::ChannelLimitMessages &limits ) {
        _limits = limits;
    }

    unsigned int Messages::_allocateMessage( unsigned int length, bool &isMemory ) {
        unsigned int id;

        if( length < _limits.minMessageSize || length > _limits.maxMessageSize ) {
            throw util::Error::EXCEED_LIMIT;
        }

        if( _totalInMemory < _limits.maxMessagesInMemory ) {
            id = _buffer->allocate( length );
            isMemory = true;
            _totalInMemory++;
        } else if( _totalOnDisk < _limits.maxMessagesOnDisk ) {
            id = _buffer->allocateOnDisk( length );
            isMemory = false;
            _totalOnDisk++;
        } else {
            throw util::Error::EXCEED_LIMIT;
        }

        return id;
    }

    unsigned int Messages::getID( const char *uuid ) {
        _wait( _countUUIDWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _mUUID );

        auto it = _uuid.find( uuid );

        if( it == _uuid.end() ) {
            throw util::Error::UNKNOWN;
        }

        return it->second;
    }

    void Messages::getUUID( unsigned int id, char *uuid ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        if( id >= _messages.size() ) {
            throw util::Error::UNKNOWN;
        }

        memcpy( uuid, _messages[id]->uuid, util::UUID::LENGTH );
    }

    void Messages::_expandMessages() {
        _messages.resize( _messages.size() + MESSAGES_IN_PACKET );
    }

    void Messages::_validateAdd( unsigned int length ) {
        if( length < _limits.minMessageSize || length > _limits.maxMessageSize ) {
            throw util::Error::WRONG_MESSAGE_SIZE;
        }
    }

    unsigned int Messages::addForQ( unsigned int length, char *uuid ) {
        _validateAdd( length );

        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        util::LockAtomic lockAtomicUUID( _countUUIDWrited );
        std::lock_guard<std::shared_timed_mutex> lockUUID( _mUUID );

        bool isMemory = false;
        auto id = _allocateMessage( length, isMemory );

        if( id >= _messages.size() ) {
            _expandMessages();
        }

        _messages[id] = std::make_unique<Message>();
        _messages[id]->isMemory = isMemory;

        while( true ) {
            util::UUID::generate( uuid );
            auto it = _uuid.find( uuid );
            if( it == _uuid.end() ) {
                _uuid[uuid] = true;
                break;
            }
        }

        memcpy( _messages[id]->uuid, uuid, util::UUID::LENGTH );

        return id;
    }

    unsigned int Messages::addForReplication( unsigned int length, const char *uuid ) {
        _validateAdd( length );

        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        _wait( _countUUIDWrited );
        std::shared_lock<std::shared_timed_mutex> lockUUID( _mUUID );

        bool isMemory = false;
        auto id = _allocateMessage( length, isMemory );

        auto itUUID = _uuid.find( uuid );
        if( itUUID != _uuid.end() ) {
            throw util::Error::DUPLICATE_UUID;
        }

        if( id >= _messages.size() ) {
            _expandMessages();
        }

        _messages[id] = std::make_unique<Message>();
        _messages[id]->isMemory = isMemory;

        memcpy( _messages[id]->uuid, uuid, util::UUID::LENGTH );

        return id;
    }

    unsigned int Messages::addForBroadcast( unsigned int length ) {
        _validateAdd( length );

        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        bool isMemory = false;
        auto id = _allocateMessage( length, isMemory );

        if( id >= _messages.size() ) {
            _expandMessages();
        }

        _messages[id] = std::make_unique<Message>();
        _messages[id]->isMemory = isMemory;

        return id;
    }

    void Messages::free( unsigned int id ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        if( id >= _messages.size() || _messages[id] == nullptr ) {
            return;
        }

        _buffer->free( id );

        if( _messages[id]->uuid[0] != 0 ) {
            auto itUUID = _uuid.find( _messages[id]->uuid );
            if( itUUID != _uuid.end() ) {
                _uuid.erase( itUUID );
                _messages[id].reset();
            }
        }
    }

    void Messages::free( const char *uuid ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        util::LockAtomic lockAtomicUUID( _countUUIDWrited );
        std::lock_guard<std::shared_timed_mutex> lockUUID( _mUUID );

        auto it = _uuid.find( uuid );
        if( it == _uuid.end() ) {
            return;
        }

        auto id = it->second;
        _uuid.erase( it );

        _buffer->free( id );
        _messages[id].reset();
    }

    unsigned int Messages::recv( unsigned int id, unsigned int fd ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        if( id >= _messages.size() || _messages[id] == nullptr ) {
            throw util::Error::UNKNOWN;
        }

        return _buffer->recv( id, fd );
    }

    unsigned int Messages::send( unsigned int id, unsigned int fd ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        if( id >= _messages.size() || _messages[id] == nullptr ) {
            throw util::Error::UNKNOWN;
        }

        return _buffer->send( id, fd );
    }

    void Messages::resetSend( unsigned int id ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        _buffer->resetSend( id );
    }
}

#endif
