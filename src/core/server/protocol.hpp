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
            const static unsigned int VERSION = 101;
            const static unsigned int SIZE_UINT = sizeof( unsigned int );
            const static unsigned int PASSWORD_LENGTH = crypto::HASH_LENGTH;

        public:
            const static unsigned int LENGTH_META = SIZE_UINT * 2;

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
            static const unsigned int SIZE_CMD = sizeof( Cmd );

            static bool _send( unsigned int fd, Packet *packet );
            static bool _recv( unsigned int fd, Packet *packet );
            static unsigned int _calculateLengthBodyMessage( unsigned int value, ... );

            static void _marsh( Packet *packet, unsigned int value );
            static void _marsh( Packet *packet, Cmd value );
            static void _marsh( Packet *packet, const char *value );

            static void _demarsh( const char *data, unsigned int &value );

            static void _reservePacketValues( Packet *packet, unsigned int length );

            static void _checkMeta( Packet *packet );
            static void _checkBody( Packet *packet );

            static unsigned int _getLengthByOffset( Packet *packet, unsigned int offset );

            static unsigned int _checkParamCmdUInt(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdPassword(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdGroupName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdChannelName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdConsumerName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdProducerName(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );
            static unsigned int _checkParamCmdUUID(
                Packet *packet,
                unsigned int offset,
                unsigned int iterator
            );

            static void _checkControlLength( unsigned int calculateLength, unsigned int length );

            static void _checkCmdCheckVersion( Packet *packet );
            static void _checkCmdUpdatePassword( Packet *packet );
            static void _checkCmdAuthGroup( Packet *packet );
            static void _checkCmdAuthConsumer( Packet *packet );
            static void _checkCmdAuthProducer( Packet *packet );
            static void _checkCmdGetUsers( Packet *packet );
            static void _checkCmdGetChannelLimitMessages( Packet *packet );
            static void _checkCmdAddChannel( Packet *packet );
            static void _checkCmdUpdateChannelLimitMessages( Packet *packet );
            static void _checkCmdRemoveChannel( Packet *packet );
            static void _checkCmdAddConsumer( Packet *packet );
            static void _checkCmdUpdateConsumerPassword( Packet *packet );
            static void _checkCmdRemoveConsumer( Packet *packet );
            static void _checkCmdAddProducer( Packet *packet );
            static void _checkCmdUpdateProducerPassword( Packet *packet );
            static void _checkCmdRemoveProducer( Packet *packet );
            static void _checkCmdPushMessage( Packet *packet );
            static void _checkCmdPushReplicaMessage( Packet *packet );
            static void _checkCmdPushPublicMessage( Packet *packet );
            static void _checkCmdRemoveMessageByUUID( Packet *packet );

        public:
            static bool sendNoSecure( unsigned int fd, Packet *packet );
            static bool sendVersion( unsigned int fd, Packet *packet );
            static bool sendOk( unsigned int fd, Packet *packet );
            static bool sendError(
                unsigned int fd,
                Packet *packet,
                const char *description
            );
            static bool sendStringList(
                unsigned int fd,
                Packet *packet,
                std::list<std::string> &list
            );
            static bool sendChannelLimitMessages(
                unsigned int fd,
                Packet *packet,
                util::types::ChannelLimitMessages &limitMessages
            );

            static bool sendMessageMetaPush(
                unsigned int fd,
                Packet *packet,
                const char *uuid
            );
            static bool sendPublicMessageMetaPush(
                unsigned int fd,
                Packet *packet
            );
            static bool sendMessageMetaPop(
                unsigned int fd,
                Packet *packet,
                unsigned int length,
                const char *uuid
            );
            static bool sendPublicMessageMetaPop(
                unsigned int fd,
                Packet *packet,
                unsigned int length
            );

            static bool continueSend( unsigned int fd, Packet *packet );

            static void recv( unsigned int fd, Packet *packet );

            static bool isOk( Packet *packet );

            static bool isCheckSecure( Packet *packet );
            static bool isCheckNoSecure( Packet *packet );
            static bool isCheckVersion( Packet *packet );

            static bool isUpdatePassword( Packet *packet );

            static bool isGetChannels( Packet *packet );
            static bool isGetChannelLimitMessages( Packet *packet );
            static bool isGetConsumers( Packet *packet );
            static bool isGetProducers( Packet *packet );

            static bool isAuthGroup( Packet *packet );
            static bool isAuthConsumer( Packet *packet );
            static bool isAuthProducer( Packet *packet );

            static bool isAddChannel( Packet *packet );
            static bool isUpdateChannelLimitMessages( Packet *packet );
            static bool isRemoveChannel( Packet *packet );

            static bool isAddConsumer( Packet *packet );
            static bool isUpdateConsumerPassword( Packet *packet );
            static bool isRemoveConsumer( Packet *packet );

            static bool isAddProducer( Packet *packet );
            static bool isUpdateProducerPassword( Packet *packet );
            static bool isRemoveProducer( Packet *packet );

            static bool isPopMessage( Packet *packet );

            static bool isPushMessage( Packet *packet );
            static bool isPushPublicMessage( Packet *packet );
            static bool isPushReplicaMessage( Packet *packet );

            static bool isRemoveMessage( Packet *packet );
            static bool isRemoveMessageByUUID( Packet *packet );

            static const char *getGroup( Packet *packet );
            static const char *getChannel( Packet *packet );
            static const char *getConsumer( Packet *packet );
            static const char *getProducer( Packet *packet );

            static const unsigned char *getPassword( Packet *packet );
            static const unsigned char *getNewPassword( Packet *packet );

            static unsigned int getVersion( Packet *packet );

            static unsigned int getLength( Packet *packet );
            static const char *getUUID( Packet *packet );
            static void getChannelLimitMessages(
                Packet *packet,
                util::types::ChannelLimitMessages &limitMessages
            );

            static bool isReceived( Packet *packet );
    };

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

    bool Protocol::continueSend( unsigned int fd, Packet *packet ) {
        return _send( fd, packet );
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

    void Protocol::_reservePacketValues( Packet *packet, unsigned int length ){
        packet->values = std::make_unique<char[]>( length );
        packet->length = length;
        packet->wrLength = 0;
    }

    bool Protocol::sendNoSecure( unsigned int fd, Packet *packet ) {
        _reservePacketValues( packet, LENGTH_META );

        _marsh( packet, CMD_CHECK_NOSECURE );
        _marsh( packet, ( unsigned int )0 );

        return _send( fd, packet );
    }

    bool Protocol::sendVersion( unsigned int fd, Packet *packet ) {
        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, 0 );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

        _marsh( packet, CMD_CHECK_VERSION );
        _marsh( packet, lengthBody );

        _marsh( packet, 1 );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, VERSION );

        return _send( fd, packet );
    }

    bool Protocol::sendOk( unsigned int fd, Packet *packet ) {
        _reservePacketValues( packet, LENGTH_META );

        _marsh( packet, CMD_OK );
        _marsh( packet, ( unsigned int )0 );

        return _send( fd, packet );
    }

    bool Protocol::sendError( unsigned int fd, Packet *packet, const char *description ) {
        auto lengthDescr = strlen( description ) + 1;
        auto lengthBody = _calculateLengthBodyMessage( lengthDescr, 0 );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

        _marsh( packet, CMD_CHECK_VERSION );
        _marsh( packet, lengthBody );

        _marsh( packet, 1 );
        _marsh( packet, lengthDescr );
        _marsh( packet, description );

        return _send( fd, packet );
    }

    bool Protocol::sendStringList(
        unsigned int fd,
        Packet *packet,
        std::list<std::string> &list
    ) {
        auto lengthBody = SIZE_UINT;

        for( auto it = list.begin(); it != list.end(); it++ ) {
            lengthBody += SIZE_UINT;
            lengthBody += strlen( (*it).c_str() ) + 1;
        }

        _reservePacketValues( packet, LENGTH_META + lengthBody );

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
        Packet *packet,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        auto lengthBody = _calculateLengthBodyMessage(
            SIZE_UINT,
            SIZE_UINT,
            SIZE_UINT,
            SIZE_UINT,
            0
        );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

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

    bool Protocol::sendMessageMetaPush(
        unsigned int fd,
        Packet *packet,
        const char *uuid
    ) {
        auto lengthUUID = strlen( uuid ) + 1;
        auto lengthBody = _calculateLengthBodyMessage( lengthUUID, 0 );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_UUID_MESSAGE );
        _marsh( packet, lengthBody );
        _marsh( packet, 1 );
        _marsh( packet, lengthUUID );
        _marsh( packet, uuid );

        return _send( fd, packet );
    }

    bool Protocol::sendPublicMessageMetaPush( unsigned int fd, Packet *packet ) {
        return sendOk( fd, packet );
    }

    bool Protocol::sendMessageMetaPop(
        unsigned int fd,
        Packet *packet,
        unsigned int length,
        const char *uuid
    ) {
        auto lengthUUID = strlen( uuid ) + 1;
        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, lengthUUID, 0 );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

        _marsh( packet, CMD_SEND_MESSAGE_META );
        _marsh( packet, lengthBody );
        _marsh( packet, 2 );
        _marsh( packet, SIZE_UINT );
        _marsh( packet, length );
        _marsh( packet, lengthUUID );
        _marsh( packet, uuid );

        return _send( fd, packet );
    }

    bool Protocol::sendPublicMessageMetaPop(
        unsigned int fd,
        Packet *packet,
        unsigned int length
    ) {
        auto lengthBody = _calculateLengthBodyMessage( SIZE_UINT, 0 );

        _reservePacketValues( packet, LENGTH_META + lengthBody );

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
            throw util::Error::WRONG_PARAM;
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
            throw util::Error::WRONG_PASSWORD;
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
            throw util::Error::WRONG_GROUP;
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
            throw util::Error::WRONG_CHANNEL;
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
            throw util::Error::WRONG_CONSUMER;
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
            throw util::Error::WRONG_PRODUCER;
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
            throw util::Error::WRONG_UUID;
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
            throw util::Error::WRONG_CHANNEL_LIMIT_MESSAGES;
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
        offset += _checkParamCmdUUID( packet, offset, 1 );

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

    void Protocol::recv( unsigned int fd, Packet *packet ) {
        if( !packet->isRecvMeta && !packet->isRecvBody ) {
            _reservePacketValues( packet, LENGTH_META );
            packet->isRecvMeta = true;
        }

        if( packet->isRecvMeta ) {
            if( !_recv( fd, packet ) ) {
                return;
            }

            packet->isRecvMeta = false;
            packet->isRecvBody = true;
            _checkMeta( packet );
            _reservePacketValues( packet, packet->length );

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

    bool Protocol::isOk( Packet *packet ) {
        return packet->cmd == CMD_OK;
    }

    bool Protocol::isCheckSecure( Packet *packet ) {
        return packet->cmd == CMD_CHECK_SECURE;
    }

    bool Protocol::isCheckNoSecure( Packet *packet ) {
        return packet->cmd == CMD_CHECK_NOSECURE;
    }

    bool Protocol::isCheckVersion( Packet *packet ) {
        return packet->cmd == CMD_CHECK_VERSION;
    }

    bool Protocol::isUpdatePassword( Packet *packet ) {
        return packet->cmd == CMD_UPDATE_PASSWORD;
    }

    bool Protocol::isGetChannels( Packet *packet ) {
        return packet->cmd == CMD_GET_CHANNELS;
    }

    bool Protocol::isGetChannelLimitMessages( Packet *packet ) {
        return packet->cmd == CMD_GET_CHANNEL_LIMIT_MESSSAGES;
    }

    bool Protocol::isGetConsumers( Packet *packet ) {
        return packet->cmd == CMD_GET_CONUMERS;
    }

    bool Protocol::isGetProducers( Packet *packet ) {
        return packet->cmd == CMD_GET_PRODUCERS;
    }

    bool Protocol::isAuthGroup( Packet *packet ) {
        return packet->cmd == CMD_AUTH_GROUP;
    }

    bool Protocol::isAuthConsumer( Packet *packet ) {
        return packet->cmd == CMD_AUTH_CONSUMER;
    }

    bool Protocol::isAuthProducer( Packet *packet ) {
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
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isAuthGroup( packet ) ) {
            return &values[offsets[0]];
        } else if( isAuthConsumer( packet ) ) {
            return &values[offsets[0]];
        } else if( isAuthProducer( packet ) ) {
            return &values[offsets[0]];
        }

        throw util::Error::WRONG_CMD;
    }

    const char *Protocol::getChannel( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isAuthGroup( packet ) ) {
            return &values[offsets[1]];
        } else if( isAuthConsumer( packet ) ) {
            return &values[offsets[1]];
        } else if( isAuthProducer( packet ) ) {
            return &values[offsets[1]];
        } else if( isGetConsumers( packet ) ) {
            return &values[offsets[0]];
        } else if( isGetProducers( packet ) ) {
            return &values[offsets[0]];
        } else if( isAddChannel( packet ) ) {
            return &values[offsets[0]];
        } else if( isUpdateChannelLimitMessages( packet ) ) {
            return &values[offsets[0]];
        } else if( isRemoveChannel( packet ) ) {
            return &values[offsets[0]];
        } else if( isAddConsumer( packet ) ) {
            return &values[offsets[0]];
        } else if( isUpdateConsumerPassword( packet ) ) {
            return &values[offsets[0]];
        } else if( isRemoveConsumer( packet ) ) {
            return &values[offsets[0]];
        } else if( isAddProducer( packet ) ) {
            return &values[offsets[0]];
        } else if( isUpdateProducerPassword( packet ) ) {
            return &values[offsets[0]];
        } else if( isRemoveProducer( packet ) ) {
            return &values[offsets[0]];
        } else if( isGetChannelLimitMessages( packet ) ) {
            return &values[offsets[0]];
        }

        throw util::Error::WRONG_CMD;
    }

    const char *Protocol::getConsumer( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isAuthConsumer( packet ) ) {
            return &values[offsets[2]];
        } else if( isAddConsumer( packet ) ) {
            return &values[offsets[1]];
        } else if( isUpdateConsumerPassword( packet ) ) {
            return &values[offsets[1]];
        } else if( isRemoveConsumer( packet ) ) {
            return &values[offsets[1]];
        }

        throw util::Error::WRONG_CMD;
    }

    const char *Protocol::getProducer( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isAuthProducer( packet ) ) {
            return &values[offsets[2]];
        } else if( isAddProducer( packet ) ) {
            return &values[offsets[1]];
        } else if( isUpdateProducerPassword( packet ) ) {
            return &values[offsets[1]];
        } else if( isRemoveProducer( packet ) ) {
            return &values[offsets[1]];
        }

        throw util::Error::WRONG_CMD;
    }

    const unsigned char *Protocol::getPassword( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isAuthGroup( packet ) ) {
            return (const unsigned char *)&values[offsets[1]];
        } else if( isAuthConsumer( packet ) ) {
            return (const unsigned char *)&values[offsets[3]];
        } else if( isAuthProducer( packet ) ) {
            return (const unsigned char *)&values[offsets[3]];
        } else if( isUpdatePassword( packet ) ) {
            return (const unsigned char *)&values[offsets[0]];
        } else if( isAddConsumer( packet ) ) {
            return (const unsigned char *)&values[offsets[2]];
        } else if( isAddProducer( packet ) ) {
            return (const unsigned char *)&values[offsets[2]];
        } else if( isUpdateConsumerPassword( packet ) ) {
            return (const unsigned char *)&values[offsets[2]];
        } else if( isUpdateProducerPassword( packet ) ) {
            return (const unsigned char *)&values[offsets[2]];
        }
        throw util::Error::WRONG_CMD;
    }

    const unsigned char *Protocol::getNewPassword( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isUpdatePassword( packet ) ) {
            return (const unsigned char *)&values[offsets[1]];
        }

        throw util::Error::WRONG_CMD;
    }

    unsigned int Protocol::getVersion( Packet *packet ) {
        if( isCheckVersion( packet ) ) {
            unsigned int value = 0;
            _demarsh( &packet->values[packet->valuesOffsets[0]], value );

            return value;
        }

        throw util::Error::WRONG_CMD;
    }

    unsigned int Protocol::getLength( Packet *packet ) {
        if( isPushMessage( packet ) || isPushPublicMessage( packet ) || isPushReplicaMessage ( packet ) ) {
            unsigned int value = 0;
            _demarsh( &packet->values[packet->valuesOffsets[0]], value );

            return value;
        }

        throw util::Error::WRONG_CMD;
    }

    const char *Protocol::getUUID( Packet *packet ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isPushReplicaMessage( packet ) ) {
            return &values[offsets[1]];
        } else if( isRemoveMessageByUUID( packet ) ) {
            return &values[offsets[0]];
        }

        throw util::Error::WRONG_CMD;
    }

    void Protocol::getChannelLimitMessages(
        Packet *packet,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        auto values = packet->values.get();
        auto offsets = packet->valuesOffsets.get();

        if( isUpdateChannelLimitMessages( packet ) || isAddChannel( packet ) ) {
            _demarsh( &values[offsets[1]], limitMessages.minMessageSize );
            _demarsh( &values[offsets[2]], limitMessages.maxMessageSize );
            _demarsh( &values[offsets[3]], limitMessages.maxMessagesInMemory );
            _demarsh( &values[offsets[4]], limitMessages.maxMessagesOnDisk );

            return;
        }

        throw util::Error::WRONG_CMD;
    }

    bool Protocol::isReceived( Packet *packet ) {
        bool isPassedMeta = !packet->isRecvMeta && packet->isRecvBody;
        bool isPassedLength = packet->length == 0 || packet->length == packet->wrLength;

        return isPassedMeta && isPassedLength;
    }
}

#endif
 
