#ifndef SIMQ_CORE_SERVER_Q_MESSAGES
#define SIMQ_CORE_SERVER_Q_MESSAGES

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <string.h>
#include "../../../util/uuid.hpp"
#include "../../../util/lock_atomic.hpp"
#include "../../../util/buffer.hpp"
#include "../../../util/types.h"
#include "../../../util/error.h"
#include "../../../util/constants.h"

namespace simq::core::server::q {
    class Messages {
        public:
            struct WRData {
                unsigned int length;
                unsigned int wrLength;
            };
        private:
            const unsigned int MESSAGES_IN_PACKET = 10'000;
            const unsigned int MESSAGE_PACKET_SIZE = util::constants::MESSAGE_PACKET_SIZE;
            util::Buffer *_buffer;

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
            unsigned int _total = MESSAGES_IN_PACKET;
            util::types::ChannelLimitMessages _limits;

            Message **_messages = nullptr;

            void _wait( std::atomic_uint &atom );
            unsigned int _allocateMessage( unsigned int length, bool &isMemory );
            void _expandMessages();
            unsigned int _calculateWRLength( WRData &wrData );
            void _validateAdd( unsigned int length );
        public:
            Messages( const char *path, util::types::ChannelLimitMessages &limits );
            ~Messages();

            void updateLimits( util::types::ChannelLimitMessages &limits );

            unsigned int addForQ( unsigned int length, char *uuid, WRData &wrData );
            unsigned int addForReplication( unsigned int length, const char *uuid, WRData &wrData );
            unsigned int addForBroadcast( unsigned int length, WRData &wrData );
            void free( unsigned int id );
            void free( const char *uuid );
            void getUUID( unsigned int id, char *uuid );
            unsigned int getID( const char *uuid );

            void recv( unsigned int id, unsigned int fd, WRData &wrData );
            void send( unsigned int id, unsigned int fd, WRData &wrData );
            bool isFullPart( WRData &wrData );
    };

    Messages::Messages( const char *path, util::types::ChannelLimitMessages &limits ) {
        _buffer = new util::Buffer( path );
        _messages = new Message*[MESSAGES_IN_PACKET];
        _limits = limits;
    }

    Messages::~Messages() {
        delete _buffer;

        for( unsigned int i = 0; i < _total; i++ ) {
            if( _messages[i] != nullptr ) {
                delete _messages[i];
            }
        }

        delete[] _messages;
    }

    void Messages::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void Messages::updateLimits( util::types::ChannelLimitMessages &limits ) {
        _limits = limits;
    }

    unsigned int Messages::_allocateMessage( unsigned int length, bool &isMemory ) {
        unsigned int id;

        if( _totalInMemory < _limits.maxMessagesInMemory ) {
            id = _buffer->allocate( length < MESSAGE_PACKET_SIZE ? length : MESSAGE_PACKET_SIZE );
            isMemory = true;
            _totalInMemory++;
        } else if( _totalOnDisk < _limits.maxMessagesOnDisk ) {
            id = _buffer->allocateOnDisk();
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

        if( id >= _total ) {
            throw util::Error::UNKNOWN;
        }

        memcpy( uuid, _messages[id]->uuid, util::UUID::LENGTH );
    }

    void Messages::_expandMessages() {
        auto tmp = new Message*[_total + MESSAGES_IN_PACKET]{};
        memcpy( tmp, _messages, _total * sizeof( Message * ) );
        delete[] _messages;

        _messages = tmp;
        _total += MESSAGES_IN_PACKET;
    }

    void Messages::_validateAdd( unsigned int length ) {
        if( length < _limits.minMessageSize || length > _limits.maxMessageSize ) {
            throw util::Error::WRONG_MESSAGE_SIZE;
        }
    }

    unsigned int Messages::addForQ( unsigned int length, char *uuid, WRData &wrData ) {
        _validateAdd( length );

        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        util::LockAtomic lockAtomicUUID( _countUUIDWrited );
        std::lock_guard<std::shared_timed_mutex> lockUUID( _mUUID );

        bool isMemory = false;
        auto id = _allocateMessage( length, isMemory );

        auto msg = new Message{};
        msg->isMemory = isMemory;

        if( id >= _total ) {
            _expandMessages();
        }

        _messages[id] = msg;

        wrData.length = length;
        wrData.wrLength = 0;

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

    unsigned int Messages::addForReplication( unsigned int length, const char *uuid, WRData &wrData ) {
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

        auto msg = new Message{};
        msg->isMemory = isMemory;

        if( id >= _total ) {
            _expandMessages();
        }

        _messages[id] = msg;

        wrData.length = length;
        wrData.wrLength = 0;

        memcpy( _messages[id]->uuid, uuid, util::UUID::LENGTH );

        return id;
    }

    unsigned int Messages::addForBroadcast( unsigned int length, WRData &wrData ) {
        _validateAdd( length );

        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        bool isMemory = false;
        auto id = _allocateMessage( length, isMemory );

        auto msg = new Message{};
        msg->isMemory = isMemory;

        if( id >= _total ) {
            _expandMessages();
        }

        _messages[id] = msg;

        wrData.length = length;
        wrData.wrLength = 0;

        return id;
    }

    void Messages::free( unsigned int id ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        if( id >= _total || _messages[id] == nullptr ) {
            return;
        }

        _buffer->free( id );

        if( _messages[id]->uuid[0] != 0 ) {
            auto itUUID = _uuid.find( _messages[id]->uuid );
            if( itUUID != _uuid.end() ) {
                _uuid.erase( itUUID );
            }
        }

        delete _messages[id];
        _messages[id] = nullptr;
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

        delete _messages[id];
        _messages[id] = nullptr;
    }

    unsigned int Messages::_calculateWRLength( WRData &wrData ) {
        auto length = 0;

        if( wrData.length < MESSAGE_PACKET_SIZE ) {
            length = wrData.length - wrData.wrLength;
        } else {
            unsigned int count = wrData.wrLength / MESSAGE_PACKET_SIZE;
            unsigned int fullCount = wrData.length / MESSAGE_PACKET_SIZE;
            if( fullCount == count ) {
                length = wrData.length - wrData.wrLength;
            } else {
                unsigned int residue = wrData.wrLength - count * MESSAGE_PACKET_SIZE;
                length = MESSAGE_PACKET_SIZE - residue;
            }
        }

        return length;
    }

    void Messages::recv( unsigned int id, unsigned int fd, WRData &wrData ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        if( id >= _total || _messages[id] == nullptr ) {
            throw util::Error::UNKNOWN;
        }

        wrData.wrLength += _buffer->recv( id, fd, _calculateWRLength( wrData ) );
    }

    void Messages::send( unsigned int id, unsigned int fd, WRData &wrData ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        if( id >= _total || _messages[id] == nullptr ) {
            throw util::Error::UNKNOWN;
        }

        wrData.wrLength += _buffer->send( id, fd, _calculateWRLength( wrData ), wrData.wrLength );
    }

    bool Messages::isFullPart( WRData &wrData ) {
        return wrData.wrLength % MESSAGE_PACKET_SIZE == 0 || wrData.wrLength == wrData.length;
    }
}

#endif
