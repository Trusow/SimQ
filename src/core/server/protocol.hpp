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
#include "../../util/validation.hpp"
#include "../../crypto/hash.hpp"

// NO SAFE THREAD!!!

namespace simq::core::server {
    class Protocol {
        private:
            const unsigned int VERSION = 101;
            const unsigned int SIZE_UINT = sizeof( unsigned int );
            const unsigned int PASSWORD_LENGTH = crypto::HASH_LENGTH;

        public:
            const unsigned int LENGTH_META = SIZE_UINT * 2;

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
                CMD_REMOVE_CONSUMER = 4'003,

                CMD_ADD_PRODUCER = 5'001,
                CMD_UPDATE_PRODUCER_PASSWORD = 5'002,
                CMD_REMOVE_PRODUCER = 5'003,

                CMD_PUSH_MESSAGE = 6'001,
                CMD_PUSH_REPLICA_MESSAGE = 6'002,
                CMD_PUSH_PUBLIC_MESSAGE = 6'003,
                CMD_SEND_UUID_MESSAGE = 6'004,

                CMD_REMOVE_MESSAGE = 6'101,
                CMD_REMOVE_MESSAGE_BY_UUID = 6'102,

                CMD_POP_MESSAGE = 6'201,

                CMD_SEND_MESSAGE_META = 6'301,
                CMD_SEND_PUBLIC_MESSAGE_META = 6'302,
            };

            struct Packet {
                Cmd cmd;
                unsigned int length;
                unsigned int wrLength;

                bool isRecvMeta;
                bool isRecvBody;

                unsigned int countValues;
                std::unique_ptr<unsigned int[]> valuesOffsets;
                std::unique_ptr<char[]> values;
            };
        private:
            const unsigned int SIZE_CMD = sizeof( Cmd );

            std::map<unsigned int, std::unique_ptr<Packet>> _packets;
            void _checkNoIsset( unsigned int fd );
            void _checkIsset( unsigned int fd );
            bool _send( unsigned int fd, Packet *packet );
            bool _recv( unsigned int fd, Packet *packet );
            unsigned int _calculateLengthBodyMessage( unsigned int value, ... );

            void _marsh( Packet *packet, unsigned int value );
            void _marsh( Packet *packet, Cmd value );
            void _marsh( Packet *packet, const char *value );

            void _demarsh( const char *data, unsigned int &value );

            void _checkMeta( Packet *packet );
            void _checkBody( Packet *packet );

            unsigned int _getLengthByOffset( Packet *packet, unsigned int offset );

            unsigned int _checkParamCmdUInt(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdPassword(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdGroupName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdChannelName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdConsumerName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdProducerName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            unsigned int _checkParamCmdUUID(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );

            void _checkControlLength( unsigned int calculateLength, unsigned int length );

            void _checkCmdCheckVersion( Packet *packet );
            void _checkCmdUpdatePassword( Packet *packet );
            void _checkCmdAuthGroup( Packet *packet );
            void _checkCmdAuthConsumer( Packet *packet );
            void _checkCmdAuthProducer( Packet *packet );
            void _checkCmdGetUsers( Packet *packet );
            void _checkCmdGetChannelLimitMessages( Packet *packet );
            void _checkCmdAddChannel( Packet *packet );
            void _checkCmdUpdateChannelLimitMessages( Packet *packet );
            void _checkCmdRemoveChannel( Packet *packet );
            void _checkCmdAddConsumer( Packet *packet );
            void _checkCmdUpdateConsumerPassword( Packet *packet );
            void _checkCmdRemoveConsumer( Packet *packet );
            void _checkCmdAddProducer( Packet *packet );
            void _checkCmdUpdateProducerPassword( Packet *packet );
            void _checkCmdRemoveProducer( Packet *packet );
            void _checkCmdPushMessage( Packet *packet );
            void _checkCmdPushReplicaMessage( Packet *packet );
            void _checkCmdPushPublicMessage( Packet *packet );
            void _checkCmdRemoveMessageByUUID( Packet *packet );

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

            bool isCmdCheckSecure( Packet *packet );
            bool isCmdCheckNoSecure( Packet *packet );
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
            const char *getOldPassword( Packet *packet );
            const char *getNewPassword( Packet *packet );

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
            &packet->values.get()[packet->wrLength],
            packet->length - packet->wrLength,
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

    bool Protocol::_recv( unsigned int fd, Packet *packet ) {
        auto l = ::recv(
            fd,
            &packet->values.get()[packet->wrLength],
            packet->length - packet->wrLength,
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
        unsigned int total = SIZE_UINT;
        va_list args;
        va_start( args, value );
 
        while( value-- ) {
            auto item = va_arg( args, unsigned int );
            if( item == 0 ) {
                break;
            }

            total += SIZE_UINT;
            total += item;
        }

        return total;
    }

    void Protocol::_marsh( Packet *packet, unsigned int value ) {
        auto v = htonl( value );
        auto size = SIZE_UINT;
        memcpy( &packet->values[packet->length], &v, size );
        packet->length += size;
    }

    void Protocol::_marsh( Packet *packet, Cmd value ) {
        auto v = htonl( value );
        auto size = SIZE_CMD;
        memcpy( &packet->values[packet->length], &v, size );
        packet->length += SIZE_CMD;
    }

    void Protocol::_marsh( Packet *packet, const char *value ) {
        auto l = strlen( value );
        packet->length += strlen( value );
        memcpy( &packet->values[packet->length], value, l );
        packet->length += l;
    }

    void Protocol::_demarsh( const char *data, unsigned int &value ) {
        memcpy( &value, data, SIZE_UINT );
        value = ntohl( value );
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

        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_CHECK_VERSION );
        _marsh( packet, lengthBody );

        _marsh( packet, 1 );
        _marsh( packet, SIZE_UINT );
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

        auto lengthBody = SIZE_UINT;

        for( auto it = list.begin(); it != list.end(); it++ ) {
            lengthBody += SIZE_UINT;
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
            SIZE_UINT,
            SIZE_UINT,
            SIZE_UINT,
            SIZE_UINT,
            0
        );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_CHANNEL_LIMIT_MESSSAGES );
        _marsh( packet, lengthBody );
        _marsh( packet, 4 );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, limitMessages.minMessageSize );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, limitMessages.maxMessageSize );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, limitMessages.maxMessagesInMemory );
        _marsh( packet, SIZE_UINT );
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

        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, lengthUUID, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_MESSAGE_META );
        _marsh( packet, lengthBody );
        _marsh( packet, 2 );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, length );
        _marsh( packet, lengthUUID );
        _marsh( packet, uuid );

        return _send( fd, packet );
    }

    bool Protocol::sendPublicMessageMetaPop( unsigned int fd, unsigned int length ) {
        _checkIsset( fd );
        _packets[fd].reset();

        auto packet = _packets[fd].get();

        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, 0 );

        packet->values = std::make_unique<char[]>( LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_MESSAGE_META );
        _marsh( packet, lengthBody );
        _marsh( packet, 1 );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, length );

        return _send( fd, packet );
    }

    void Protocol::_checkMeta( Packet *packet ) {
        memcpy( &packet->cmd, packet->values.get(), SIZE_CMD );
        memcpy( &packet->length, &packet->values.get()[SIZE_CMD], SIZE_UINT );
        packet->cmd = ( Cmd )ntohl( packet->cmd );
        packet->length = ntohl( packet->length );

        switch( packet->cmd ) {
            case CMD_OK:
            case CMD_CHECK_SECURE:
            case CMD_CHECK_NOSECURE:
            case CMD_REMOVE_MESSAGE:
            case CMD_GET_CHANNELS:
                if( packet->length != 0 ) {
                    throw util::Error::WRONG_CMD;
                }
                break;
            case CMD_CHECK_VERSION:
            case CMD_UPDATE_PASSWORD:
            case CMD_AUTH_GROUP:
            case CMD_AUTH_CONSUMER:
            case CMD_AUTH_PRODUCER:
            case CMD_GET_CONUMERS:
            case CMD_GET_PRODUCERS:
            case CMD_GET_CHANNEL_LIMIT_MESSSAGES:
            case CMD_ADD_CHANNEL:
            case CMD_UPDATE_CHANNEL_LIMIT_MESSAGES:
            case CMD_REMOVE_CHANNEL:
            case CMD_ADD_CONSUMER:
            case CMD_UPDATE_CONSUMER_PASSWORD:
            case CMD_REMOVE_CONSUMER:
            case CMD_ADD_PRODUCER:
            case CMD_UPDATE_PRODUCER_PASSWORD:
            case CMD_REMOVE_PRODUCER:
            case CMD_PUSH_MESSAGE:
            case CMD_PUSH_PUBLIC_MESSAGE:
            case CMD_PUSH_REPLICA_MESSAGE:
            case CMD_REMOVE_MESSAGE_BY_UUID:
                if( packet->length > 4096 ) {
                    throw util::Error::WRONG_CMD;
                }
                break;
            default:
                throw util::Error::WRONG_CMD;
                break;
        }
    }

    unsigned int Protocol::_getLengthByOffset( Packet *packet, unsigned int offset ) {
        unsigned int length = 0;
        memcpy( &length, &packet->values[offset], SIZE_UINT );

        length = ntohl( length );

        if( length == 0 || offset + SIZE_UINT + length > packet->length ) {
            throw util::Error::WRONG_CMD;
        }

        return length;
    }

    unsigned int Protocol::_checkParamCmdUInt(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        if( l != SIZE_UINT ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdPassword(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        if( l != PASSWORD_LENGTH ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdGroupName(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        auto name = &packet->values[offset+SIZE_UINT];

        if( !util::Validation::isGroupName( name ) || strlen( name ) != l-1 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdChannelName(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        auto name = &packet->values[offset+SIZE_UINT];

        if( !util::Validation::isChannelName( name ) || strlen( name ) != l-1 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdConsumerName(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        auto name = &packet->values[offset+SIZE_UINT];

        if( !util::Validation::isConsumerName( name ) || strlen( name ) != l-1 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdProducerName(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        auto name = &packet->values[offset+SIZE_UINT];

        if( !util::Validation::isProducerName( name ) || strlen( name ) != l-1 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    unsigned int Protocol::_checkParamCmdUUID(
        Packet *packet,
        unsigned int offset,
        unsigned int iterator
    ) {
        auto l = _getLengthByOffset( packet, offset );

        auto name = &packet->values[offset+SIZE_UINT];

        if( !util::Validation::isUUID( name ) || strlen( name ) != l-1 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets[iterator] = l;

        return SIZE_UINT + l;
    }

    void Protocol::_checkControlLength( unsigned int calculateLength, unsigned int length ) {
        if( calculateLength != length ) {
            throw util::Error::WRONG_CMD;
        }
    }

    void Protocol::_checkCmdCheckVersion( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdUInt( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdUpdatePassword( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdPassword( packet, offset, 0 );
        offset += _checkParamCmdPassword( packet, offset, 1 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAuthGroup( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdGroupName( packet, offset, 0 );
        offset += _checkParamCmdPassword( packet, offset, 1 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAuthConsumer( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdGroupName( packet, offset, 0 );
        offset += _checkParamCmdChannelName( packet, offset, 1 );
        offset += _checkParamCmdConsumerName( packet, offset, 2 );
        offset += _checkParamCmdPassword( packet, offset, 3 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAuthProducer( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdGroupName( packet, offset, 0 );
        offset += _checkParamCmdChannelName( packet, offset, 1 );
        offset += _checkParamCmdProducerName( packet, offset, 2 );
        offset += _checkParamCmdPassword( packet, offset, 3 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdGetUsers( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdChannelName( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdGetChannelLimitMessages( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdChannelName( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAddChannel( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdChannelName( packet, offset, 0 );
        offset += _checkParamCmdUInt( packet, offset, 1 );
        offset += _checkParamCmdUInt( packet, offset, 2 );
        offset += _checkParamCmdUInt( packet, offset, 3 );
        offset += _checkParamCmdUInt( packet, offset, 4 );

        _checkControlLength( offset, packet->length );

        util::types::ChannelLimitMessages limitMessages{};

        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        _demarsh( &values[offsets[1]], limitMessages.minMessageSize );
        _demarsh( &values[offsets[2]], limitMessages.maxMessageSize );
        _demarsh( &values[offsets[3]], limitMessages.maxMessagesInMemory );
        _demarsh( &values[offsets[4]], limitMessages.maxMessagesOnDisk );

        if( !util::Validation::isChannelLimitMessages( limitMessages ) ) {
            throw util::Error::WRONG_CMD;
        }
    }

    void Protocol::_checkCmdUpdateChannelLimitMessages( Packet *packet ) {
        _checkCmdAddChannel( packet );
    }

    void Protocol::_checkCmdRemoveChannel( Packet *packet ) {
        auto offset = SIZE_UINT;
        offset += _checkParamCmdChannelName( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAddConsumer( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdChannelName( packet, offset, 0 );
        offset += _checkParamCmdConsumerName( packet, offset, 1 );
        offset += _checkParamCmdPassword( packet, offset, 2 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdUpdateConsumerPassword( Packet *packet ) {
        _checkCmdAddConsumer( packet );
    }

    void Protocol::_checkCmdRemoveConsumer( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdChannelName( packet, offset, 0 );
        offset += _checkParamCmdConsumerName( packet, offset, 1 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdAddProducer( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdChannelName( packet, offset, 0 );
        offset += _checkParamCmdProducerName( packet, offset, 1 );
        offset += _checkParamCmdPassword( packet, offset, 2 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdUpdateProducerPassword( Packet *packet ) {
        _checkCmdAddProducer( packet );
    }

    void Protocol::_checkCmdRemoveProducer( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdChannelName( packet, offset, 0 );
        offset += _checkParamCmdProducerName( packet, offset, 1 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdPushMessage( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdUInt( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdPushReplicaMessage( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdUInt( packet, offset, 0 );
        offset += _checkParamCmdUUID( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkCmdPushPublicMessage( Packet *packet ) {
        _checkCmdPushMessage( packet );
    }

    void Protocol::_checkCmdRemoveMessageByUUID( Packet *packet ) {
        auto offset = SIZE_UINT;

        offset += _checkParamCmdUUID( packet, offset, 0 );

        _checkControlLength( offset, packet->length );
    }

    void Protocol::_checkBody( Packet *packet ) {
        memcpy( &packet->countValues, packet->values.get(), SIZE_UINT );
        packet->countValues = ntohl( packet->countValues );

        if( packet->countValues == 0 || packet->countValues > 16 ) {
            throw util::Error::WRONG_CMD;
        }

        packet->valuesOffsets = std::make_unique<unsigned int[]>( packet->countValues );

        switch( packet->cmd ) {
            case CMD_CHECK_VERSION:
                _checkCmdCheckVersion( packet );
                break;
            case CMD_UPDATE_PASSWORD:
                _checkCmdUpdatePassword( packet );
                break;
            case CMD_AUTH_GROUP:
                _checkCmdAuthGroup( packet );
                break;
            case CMD_AUTH_CONSUMER:
                _checkCmdAuthConsumer( packet );
                break;
            case CMD_AUTH_PRODUCER:
                _checkCmdAuthProducer( packet );
                break;
                break;
            case CMD_GET_CONUMERS:
            case CMD_GET_PRODUCERS:
                _checkCmdGetUsers( packet );
                break;
                break;
            case CMD_GET_CHANNEL_LIMIT_MESSSAGES:
                _checkCmdGetChannelLimitMessages( packet );
                break;
            case CMD_ADD_CHANNEL:
                _checkCmdAddChannel( packet );
                break;
            case CMD_UPDATE_CHANNEL_LIMIT_MESSAGES:
                _checkCmdUpdateChannelLimitMessages( packet );
                break;
            case CMD_REMOVE_CHANNEL:
                _checkCmdRemoveChannel( packet );
                break;
            case CMD_ADD_CONSUMER:
                _checkCmdAddConsumer( packet );
                break;
            case CMD_ADD_PRODUCER:
                _checkCmdAddProducer( packet );
                break;
            case CMD_UPDATE_CONSUMER_PASSWORD:
                _checkCmdUpdateConsumerPassword( packet );
                break;
            case CMD_UPDATE_PRODUCER_PASSWORD:
                _checkCmdUpdateProducerPassword( packet );
                break;
            case CMD_REMOVE_CONSUMER:
                _checkCmdRemoveConsumer( packet );
                break;
            case CMD_REMOVE_PRODUCER:
                _checkCmdRemoveProducer( packet );
                break;
            case CMD_PUSH_MESSAGE:
                _checkCmdPushMessage( packet );
                break;
            case CMD_PUSH_REPLICA_MESSAGE:
                _checkCmdPushReplicaMessage( packet );
                break;
            case CMD_PUSH_PUBLIC_MESSAGE:
                _checkCmdPushPublicMessage( packet );
                break;
            case CMD_REMOVE_MESSAGE_BY_UUID:
                _checkCmdRemoveMessageByUUID( packet );
                break;
        }
    }

    void Protocol::recv( unsigned int fd ) {
        _checkIsset( fd );

        auto packet = _packets[fd].get();

        if( !packet->isRecvMeta && !packet->isRecvBody ) {
            _packets[fd].reset();
            packet->values = std::make_unique<char[]>( LENGTH_META );
            packet->isRecvMeta = true;
            packet->length = LENGTH_META;
        }

        if( packet->isRecvMeta ) {
            if( !_recv( fd, packet ) ) {
                return;
            }

            packet->isRecvMeta = false;
            packet->isRecvBody = true;
            _checkMeta( packet );
            packet->wrLength = 0;
            packet->values = std::make_unique<char[]>( packet->length );

            if( !packet->length ) {
                return;
            }
        }

        if( packet->isRecvBody && packet->length != packet->wrLength ) {
            if( !_recv( fd, packet ) ) {
                return;
            }

            _checkBody( packet );

            packet->isRecvMeta = false;
            packet->isRecvBody = false;
        }
    }

    const Protocol::Packet *Protocol::getPacket( unsigned fd ) {
        _checkIsset( fd );

        auto packet = _packets[fd].get();

        bool isPassedMeta = !packet->isRecvMeta && packet->isRecvBody;
        bool isPassedLength = packet->length == 0 || packet->length == packet->wrLength;

        return isPassedMeta && isPassedLength ? packet : nullptr;
    }

    bool Protocol::isOk( Packet *packet ) {
        return packet->cmd == CMD_OK;
    }

    bool Protocol::isCmdCheckSecure( Packet *packet ) {
        return packet->cmd == CMD_CHECK_SECURE;
    }

    bool Protocol::isCmdCheckNoSecure( Packet *packet ) {
        return packet->cmd == CMD_CHECK_NOSECURE;
    }

    bool Protocol::isCmdCheckVersion( Packet *packet ) {
        return packet->cmd == CMD_CHECK_VERSION;
    }

    bool Protocol::isCmdAuthGroup( Packet *packet ) {
        return packet->cmd == CMD_AUTH_GROUP;
    }

    bool Protocol::isCmdAuthConsumer( Packet *packet ) {
        return packet->cmd == CMD_AUTH_CONSUMER;
    }

    bool Protocol::isCmdAuthProducer( Packet *packet ) {
        return packet->cmd == CMD_AUTH_PRODUCER;
    }

    bool Protocol::isAddChannel( Packet *packet ) {
        return packet->cmd == CMD_ADD_CHANNEL;
    }

    bool Protocol::isUpdateChannelLimitMessages( Packet *packet ) {
        return packet->cmd == CMD_UPDATE_CHANNEL_LIMIT_MESSAGES;
    }

    bool Protocol::isRemoveChannel( Packet *packet ) {
        return packet->cmd == CMD_REMOVE_CHANNEL;
    }

    bool Protocol::isAddConsumer( Packet *packet ) {
        return packet->cmd == CMD_ADD_CONSUMER;
    }

    bool Protocol::isUpdateConsumerPassword( Packet *packet ) {
        return packet->cmd == CMD_UPDATE_CONSUMER_PASSWORD;
    }

    bool Protocol::isRemoveConsumer( Packet *packet ) {
        return packet->cmd == CMD_REMOVE_CONSUMER;
    }

    bool Protocol::isAddProducer( Packet *packet ) {
        return packet->cmd == CMD_ADD_PRODUCER;
    }

    bool Protocol::isUpdateProducerPassword( Packet *packet ) {
        return packet->cmd == CMD_UPDATE_PRODUCER_PASSWORD;
    }

    bool Protocol::isRemoveProducer( Packet *packet ) {
        return packet->cmd == CMD_REMOVE_PRODUCER;
    }

    bool Protocol::isPopMessage( Packet *packet ) {
        return packet->cmd == CMD_POP_MESSAGE;
    }

    bool Protocol::isPushMessage( Packet *packet ) {
        return packet->cmd == CMD_PUSH_MESSAGE;
    }

    bool Protocol::isPushPublicMessage( Packet *packet ) {
        return packet->cmd == CMD_PUSH_PUBLIC_MESSAGE;
    }

    bool Protocol::isPushReplicaMessage( Packet *packet ) {
        return packet->cmd == CMD_PUSH_REPLICA_MESSAGE;
    }

    bool Protocol::isRemoveMessage( Packet *packet ) {
        return packet->cmd == CMD_REMOVE_MESSAGE;
    }

    bool Protocol::isRemoveMessageByUUID( Packet *packet ) {
        return packet->cmd == CMD_REMOVE_MESSAGE_BY_UUID;
    }

    const char *Protocol::getGroup( Packet *packet ) {
        return nullptr;
    }

    const char *Protocol::getChannel( Packet *packet ) {
        return nullptr;
    }

    const char *Protocol::getLogin( Packet *packet ) {
        return nullptr;
    }

    const char *Protocol::getOldPassword( Packet *packet ) {
        return nullptr;
    }

    const char *Protocol::getNewPassword( Packet *packet ) {
        return nullptr;
    }

    unsigned int Protocol::getVersion( Packet *packet ) {
        return 0;
    }

    unsigned int Protocol::getLength( Packet *packet ) {
        return 0;
    }

    const char *Protocol::getUUID( Packet *packet ) {
        return nullptr;
    }
}

#endif
 
