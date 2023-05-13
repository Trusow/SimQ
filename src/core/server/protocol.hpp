#ifndef SIMQ_CORE_SERVER_PROTOCOL
#define SIMQ_CORE_SERVER_PROTOCOL

#include <list>
#include <map>
#include <string>
#include <string.h>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include "../../util/types.h"
#include "../../util/error.h"
#include "../../crypto/hash.hpp"

// NO SAFE THREAD!!!

namespace simq::core::server {
    class Protocol {
        private:
            const unsigned int VERSION = 101;

        public:
            const unsigned int LENGTH_META = 8;

            enum Cmd {
                CMD_OK = 10,
                CMD_ERROR = 20,
                CMD_STRING_LIST = 30,

                CMD_CHECK_SECURE = 101,
                CMD_CHECK_NOSECURE = 102,

                CMD_CHECK_VERSION = 201,

                CMD_UPDATE_PASSWORD = 301,

                CMD_AUTH_GROUP = 1'001,
                CMD_AUTH_CONSUMER = 1'002,
                CMD_AUTH_PRODUCER = 1'003,

                CMD_GET_CHANNELS = 2'001,
                CMD_GET_CONUMERS = 2'002,
                CMD_GET_PRODUCERS = 2'003,
                CMD_GET_CHANNEL_LIMIT_MESSSAGES = 2'101,
                CMD_SEND_CHANNEL_LIMIT_MESSSAGES = 2'102,

                CMD_ADD_CHANNEL = 3'001,
                CMD_UPDATE_CHANNEL_LIMIT_MESSAGES = 3'002,
                CMD_REMOVE_CHANNEL = 3'003,

                CMD_ADD_CONSUMER = 4'001,
                CMD_UPDATE_CONSUMER_PASSWORD = 4'002,
                CMD_REMOVE_CONSUMER = 4'002,

                CMD_ADD_PRODUCER = 5'001,
                CMD_UPDATE_PRODUCER_PASSWORD = 5'002,
                CMD_REMOVE_PRODUCER = 5'002,

                CMD_PUSH_MESSAGE = 6'001,
                CMD_PUSH_PUBLIC_MESSAGE = 6'002,
                CMD_SEND_UUID_MESSAGE = 6'003,

                CMD_REMOVE_MESSAGE = 6'101,
                CMD_REMOVE_MESSAGE_BY_UUID = 6'102,

                CMD_POP_MESSAGE = 6'201,

                CMD_SEND_MESSAGE_META = 6'301,
                CMD_SEND_PUBLIC_MESSAGE_META = 6'302,
            };

            struct Packet {
                Cmd cmd;
                unsigned int length;
                unsigned int metaLength;
                unsigned int wrLength;

                unsigned int countValues;
                std::unique_ptr<unsigned int[]> valuesLength;
                std::unique_ptr<char[]> values;
            };
        private:

            std::map<unsigned int, std::unique_ptr<Packet>> _packets;
            void _checkNoIsset( unsigned int fd );
            void _checkIsset( unsigned int fd );
            bool _send( unsigned int fd, Packet *packet );
            unsigned int _calculateLengthBodyMessage( unsigned int value, ... );

            void _marsh( Packet *packet, unsigned int value );
            void _marsh( Packet *packet, Cmd value );
            void _marsh( Packet *packet, const char *value );

            void _demarsh( const char *data, unsigned int &value );
            void _demarsh( const char *data, Cmd &value );
            void _demarsh( const char *data, char *value );

        public:
            void join( unsigned int fd );
            void leave( unsigned int fd );

            bool sendNoSecure( unsigned int fd );
            bool sendVersion( unsigned int fd );
            bool sendOk( unsigned int fd );
            bool sendError( unsigned int fd, const char *description );
            bool sendStringList( unsigned int fd, std::list<std::string> &list );
            bool sendChannelLimitMessages(
                unsigned int fd,
                util::types::ChannelLimitMessages &limitMessages
            );

            bool sendMessageMetaPush( unsigned int fd, const char *uuid );
            bool sendPublicMessageMetaPush( unsigned int fd );
            bool sendMessageMetaPop( unsigned int fd, unsigned int length, const char *uuid );
            bool sendPublicMessageMetaPop( unsigned int fd, unsigned int length );

            bool continueSend( unsigned int fd );

            void recv( unsigned int fd );

            const Packet *getPacket( unsigned fd );

            bool isOk( Packet *packet );

            bool isCmdCheckSevuce( Packet *packet );
            bool isCmdCheckVersion( Packet *packet );

            bool isCmdAuthGroup( Packet *packet );
            bool isCmdAuthConsumer( Packet *packet );
            bool isCmdAuthProducer( Packet *packet );

            bool isAddChannel( Packet *packet );
            bool isUpdateChannelLimitMessages( Packet *packet );
            bool isRemoveChannel( Packet *packet );

            bool isAddConsumer( Packet *packet );
            bool isUpdateConsumerPassword( Packet *packet );
            bool isRemoveConsumer( Packet *packet );

            bool isAddProducer( Packet *packet );
            bool isUpdateProducerPassword( Packet *packet );
            bool isRemoveProducer( Packet *packet );

            bool isPopMessage( Packet *packet );

            bool isPushMessage( Packet *packet );
            bool isPushPublicMessage( Packet *packet );
            bool isPushReplicaMessage( Packet *packet );

            bool isRemoveMessage( Packet *packet );
            bool isRemoveMessageByUUID( Packet *packet );

            const char *getGroup( Packet *packet );
            const char *getChannel( Packet *packet );
            const char *getLogin( Packet *packet );
            void getOldPassword( Packet *packet, unsigned char password[crypto::HASH_LENGTH] );
            void getNewPassword( Packet *packet, unsigned char password[crypto::HASH_LENGTH] );

            bool getSecure( Packet *packet );
            unsigned int getVersion( Packet *packet );

            unsigned int getLength( Packet *packet );
            const char *getUUID( Packet *packet );
    };

    void Protocol::_checkNoIsset( unsigned int fd ) {
        if( _packets.find( fd ) != _packets.end() ) {
            throw util::Error::DUPLICATE_SESSION;
        }
    }

    void Protocol::_checkIsset( unsigned int fd ) {
        if( _packets.find( fd ) == _packets.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }
    }

    void Protocol::join( unsigned int fd ) {
        _checkNoIsset( fd );
        _packets[fd] = std::make_unique<Packet>();
    }

    void Protocol::leave( unsigned int fd ) {
        _checkIsset( fd );
        _packets[fd].reset();
    }

    bool Protocol::_send( unsigned int fd, Packet *packet ) {
        auto l = ::send(
            fd,
            packet->values.get(),
            packet->length,
            MSG_NOSIGNAL
        );

        if( l == -1 ) {
            if( errno != EAGAIN ) {
                throw util::Error::SOCKET;
            }

            return false;
        }

        packet->wrLength += l;

        return packet->wrLength == packet->length;
    }

    bool Protocol::continueSend( unsigned int fd ) {
        _checkIsset( fd );
        return _send( fd, _packets[fd].get() );
    };

    unsigned int Protocol::_calculateLengthBodyMessage( unsigned int value, ... ) {
        unsigned int total = sizeof( unsigned int );
        va_list args;
        va_start( args, value );
 
        while( value-- ) {
            auto item = va_arg( args, unsigned int );
            if( item == 0 ) {
                break;
            }

            total += sizeof( unsigned int );
            total += item;
        }

        return total;
    }

    void Protocol::_marsh( Packet *packet, unsigned int value ) {
        auto v = htonl( value );
        auto size = sizeof( value );
        memcpy( &packet->values[packet->length], &v, size );
        packet->length += size;
    }

    void Protocol::_marsh( Packet *packet, Cmd value ) {
        auto v = htonl( value );
        auto size = sizeof( value );
        memcpy( &packet->values[packet->length], &v, size );
        packet->length += sizeof( Cmd );
    }

    void Protocol::_marsh( Packet *packet, const char *value ) {
        auto l = strlen( value );
        packet->length += strlen( value );
        memcpy( &packet->values[packet->length], value, l );
        packet->length += l;
    }

    bool Protocol::sendNoSecure( unsigned int fd ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        packet->values = std::make_unique<char[]>( LENGTH_META );

        _marsh( packet, CMD_CHECK_NOSECURE );
        _marsh( packet, ( unsigned int )0 );

        return _send( fd, packet );
    }

    bool Protocol::sendVersion( unsigned int fd ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthBody = _calculateLengthBodyMessage( sizeof( unsigned int ), 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_CHECK_VERSION );
        _marsh( packet, lengthBody );

        _marsh( packet, 1 );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, VERSION );

        return _send( fd, packet );
    }

    bool Protocol::sendOk( unsigned int fd ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        packet->values = std::make_unique<char[]>( LENGTH_META );

        _marsh( packet, CMD_OK );
        _marsh( packet, ( unsigned int )0 );

        return _send( fd, packet );
    }

    bool Protocol::sendError( unsigned int fd, const char *description ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthDescr = strlen( description ) + 1;

        auto lengthBody = _calculateLengthBodyMessage( lengthDescr, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_CHECK_VERSION );
        _marsh( packet, lengthBody );

        _marsh( packet, 1 );
        _marsh( packet, lengthDescr );
        _marsh( packet, description );

        return _send( fd, packet );
    }

    bool Protocol::sendStringList( unsigned int fd, std::list<std::string> &list ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthBody = sizeof( unsigned int );

        for( auto it = list.begin(); it != list.end(); it++ ) {
            lengthBody += sizeof( unsigned int );
            lengthBody += strlen( (*it).c_str() ) + 1;
        }

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_STRING_LIST );
        _marsh( packet, lengthBody );
        _marsh( packet, list.size() );

        for( auto it = list.begin(); it != list.end(); it++ ) {
            _marsh( packet, (*it).length() + 1 );
            _marsh( packet, (*it).c_str() );
        }

        return _send( fd, packet );
    }

    bool Protocol::sendChannelLimitMessages(
        unsigned int fd,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthBody = _calculateLengthBodyMessage(
            sizeof( unsigned int ),
            sizeof( unsigned int ),
            sizeof( unsigned int ),
            sizeof( unsigned int ),
            0
        );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_CHANNEL_LIMIT_MESSSAGES );
        _marsh( packet, lengthBody );
        _marsh( packet, 4 );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, limitMessages.minMessageSize );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, limitMessages.maxMessageSize );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, limitMessages.maxMessagesInMemory );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, limitMessages.maxMessagesOnDisk );

        return _send( fd, packet );
    }

    bool Protocol::sendMessageMetaPush( unsigned int fd, const char *uuid ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthUUID = strlen( uuid ) + 1;

        auto lengthBody = _calculateLengthBodyMessage( lengthUUID, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_UUID_MESSAGE );
        _marsh( packet, lengthBody );
        _marsh( packet, 1 );
        _marsh( packet, lengthUUID );
        _marsh( packet, uuid );

        return _send( fd, packet );
    }

    bool Protocol::sendPublicMessageMetaPush( unsigned int fd ) {
        return sendOk( fd );
    }

    bool Protocol::sendMessageMetaPop( unsigned int fd, unsigned int length, const char *uuid ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthUUID = strlen( uuid ) + 1;

        auto lengthBody = _calculateLengthBodyMessage( sizeof( unsigned int ), lengthUUID, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_MESSAGE_META );
        _marsh( packet, lengthBody );
        _marsh( packet, 2 );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, length );
        _marsh( packet, lengthUUID );
        _marsh( packet, uuid );

        return _send( fd, packet );
    }

    bool Protocol::sendPublicMessageMetaPop( unsigned int fd, unsigned int length ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthBody = _calculateLengthBodyMessage( sizeof( unsigned int ), 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_MESSAGE_META );
        _marsh( packet, lengthBody );
        _marsh( packet, 1 );
        _marsh( packet, sizeof( unsigned int ) );
        _marsh( packet, length );

        return _send( fd, packet );
    }
}

#endif
 
