#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/file.hpp"
#include "../../util/fs.hpp"
#include "../../util/validation.hpp"
#include "../../util/uuid.hpp"
#include "../../util/string.hpp"
#include "../../util/constants.h"
#include <list>
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <arpa/inet.h>

namespace simq::core::server {
    class Changes {
        private:
            enum Type {
                CH_CREATE_GROUP = 1000,
                CH_UPDATE_GROUP_PASSWORD,
                CH_REMOVE_GROUP,

                CH_CREATE_CHANNEL = 2000,
                CH_UPDATE_CHANNEL_SETTINGS,
                CH_REMOVE_CHANNEL,

                CH_CREATE_CONSUMER = 3000,
                CH_UPDATE_CONSUMER_PASSWORD,
                CH_REMOVE_CONSUMER,

                CH_CREATE_PRODUCER = 4000,
                CH_UPDATE_PRODUCER_PASSWORD,
                CH_REMOVE_PRODUCER,

                CH_UPDATE_MASTER_PASSWORD = 5000,
                CH_UPDATE_PORT,
                CH_UPDATE_COUNT_THREADS,
            };
            const char *defPath = util::constants::PATH_DIR_CHANGES;
            const unsigned int LENGTH_VALUES = 16;

            struct ChangeFile {
                unsigned int type;
                unsigned int values[16];
            };

        public:

            struct Change: ChangeFile {
                char *data;
                util::types::Initiator initiator;
                char *group;
                char *channel;
                char *login;
            };

        private:

            char *_path = nullptr;

            struct ChangeDisk {
                Change *change;
                char uuid[util::UUID::LENGTH];
            };

            std::list<Change *> listMemory;
            std::list<ChangeDisk> listDisk;
            std::map<std::string, bool> unknownFiles;
            std::map<std::string, bool> passedFiles;
            std::list<std::string> failedFiles;
            std::mutex m;

            void _addChangesFromFiles();
            void _addChangesFromFile( util::File &file, const char *name );
            void _popFile();
            void _generateUniqFileName( char *name );
            bool _isAllowedType( unsigned int type );
            Change *_buildGroupChange(
                unsigned int type,
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );

            Change *_buildChannelChange(
                unsigned int type,
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );

            Change *_buildUserChange(
                unsigned int type,
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );

            char *_convertChannelLimitMessagesDataToFile( Change *change );
            char *_convertChannelLimitMessagesDataFromFile( Change *change );
            char *_convertPortDataToFile( Change *change );
            char *_convertPortDataFromFile( Change *change );
            char *_convertCountThreadsDataToFile( Change *change );
            char *_convertCountThreadsDataFromFile( Change *change );

            bool _isValidGroup( Change *change, unsigned int length );
            bool _isValidChannel( Change *change, unsigned int length );
            bool _isValidUser( Change *change, unsigned int length );
            bool _isValidUpdatePort( Change *change );
            bool _isValidUpdateCountThreads( Change *change );
            bool _isValidUpdateMasterPassword( Change *change, unsigned int length );
            bool _isValidChangeFromFile( Change *change, unsigned int length );

            void _clearFailedFiles();

        public:
            Changes( const char *path );
            ~Changes();

            Change *addGroup(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *updateGroupPassword(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *removeGroup( const char *group );

            Change *addChannel(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            Change *updateChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            Change *removeChannel( const char *group, const char *channel );

            Change *addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *removeConsumer(
                const char *group,
                const char *channel,
                const char *login
            );

            Change *addProducer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            Change *removeProducer(
                const char *group,
                const char *channel,
                const char *login
            );

            Change *updatePort( unsigned short int port );
            Change *updateCountThreads( unsigned short int port );
            Change *updateMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            );

            bool isAddGroup( Change *change );
            bool isUpdateGroupPassword( Change *change );
            bool isRemoveGroup( Change *change );

            bool isAddChannel( Change *change );
            bool isUpdateChannelLimitMessages( Change *change );
            bool isRemoveChannel( Change *change );

            bool isAddConsumer( Change *change );
            bool isUpdateConsumerPassword( Change *change );
            bool isRemoveConsumer( Change *change );

            bool isAddProducer( Change *change );
            bool isUpdateProducerPassword( Change *change );
            bool isRemoveProducer( Change *change );

            bool isUpdatePort( Change *change );
            bool isUpdateCountThreads( Change *change );
            bool isUpdateMasterPassword( Change *change );

            const char *getGroup( Change *change );
            const char *getChannel( Change *change );
            const char *getLogin( Change *change );
            const util::types::ChannelLimitMessages *getChannelLimitMessages( Change *change );
            unsigned short int getPort( Change *change );
            unsigned short int getCountThreads( Change *change );
            void getPassword( Change *change, unsigned char password[crypto::HASH_LENGTH] );

            void push(
                Change *change,
                util::types::Initiator initiator = util::types::Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
            void pushDefered( Change *change );
            Change *pop();

            util::types::Initiator getInitiator( Change *change );
            const char *getInitiatorGroup( Change *change );
            const char *getInitiatorChannel( Change *change );
            const char *getInitiatorLogin( Change *change );

            void free( Change *change );
    };

    Changes::Changes( const char *path ) {
        std::string strPath = path;
        strPath += "/";
        strPath += defPath;
        strPath += "/";
        auto _strPath = strPath.c_str();
        auto l = strPath.size();

        if( !util::FS::dirExists( _strPath ) ) {
            if( !util::FS::createDir( _strPath ) ) {
                throw util::Error::FS_ERROR;
            }
        }

        _path = new char[l+1]{};
        memcpy( _path, _strPath, l );
    }

    Changes::~Changes() {
        if( _path != nullptr ) {
            delete[] _path;
        }

        for( auto it = listMemory.begin(); it != listMemory.end(); it++ ) {
            delete[] (*it)->data;
            switch( (*it)->initiator ) {
                case util::types::Initiator::I_GROUP:
                    delete[] (*it)->group;
                    break;
                case util::types::Initiator::I_CONSUMER:
                case util::types::Initiator::I_PRODUCER:
                    delete[] (*it)->group;
                    delete[] (*it)->channel;
                    delete[] (*it)->login;
                    break;
            }
            delete *it;
        }

        for( auto it = listDisk.begin(); it != listDisk.end(); it++ ) {
            delete[] (*it).change->data;
            delete (*it).change;
        }
    }

    Changes::Change *Changes::_buildGroupChange(
        unsigned int type,
        const char *group,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        auto groupLength = strlen( group );


        auto change = new Change{};
        change->type = type;

        if( password != nullptr ) {
            change->data = new char[groupLength+1+crypto::HASH_LENGTH]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = crypto::HASH_LENGTH;
            memcpy( &change->data[groupLength+1], password, crypto::HASH_LENGTH );
        } else {
            change->data = new char[groupLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
        }

        return change;
    }

    Changes::Change *Changes::_buildChannelChange(
        unsigned int type,
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        if( !util::Validation::isChannelName( channel ) ) {
            throw util::Error::WRONG_CHANNEL;
        }

        if( limitMessages != nullptr ) {
            if( !util::Validation::isChannelLimitMessages( *limitMessages ) ) {
                throw util::Error::WRONG_SETTINGS;
            }
        }

        auto groupLength = strlen( group );
        auto channelLength = strlen( channel );
        auto limitMessagesLength = sizeof( util::types::ChannelLimitMessages );

        auto change = new Change{};
        change->type = type;

        if( limitMessages == nullptr ) {
            change->data = new char[groupLength+1+channelLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
        } else {
            change->data = new char[groupLength+1+channelLength+1+limitMessagesLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = limitMessagesLength;
            memcpy( &change->data[groupLength+1+channelLength+1], limitMessages, limitMessagesLength );
        }

        return change;
    }

    void Changes::_clearFailedFiles() {
        for( auto it = failedFiles.begin(); it != failedFiles.end(); it++ ) {
            std::string p = _path;
            p += *it;

            util::FS::removeFile( p.c_str() );
        }

        failedFiles.clear();
    }

    Changes::Change *Changes::_buildUserChange(
        unsigned int type,
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        if( !util::Validation::isChannelName( channel ) ) {
            throw util::Error::WRONG_CHANNEL;
        }

        switch( type ) {
            case CH_CREATE_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
                if( !util::Validation::isConsumerName( login ) ) {
                    throw util::Error::WRONG_LOGIN;
                }
            break;
            default:
                if( !util::Validation::isProducerName( login ) ) {
                    throw util::Error::WRONG_LOGIN;
                }
            break;
        }

        auto groupLength = strlen( group );
        auto channelLength = strlen( channel );
        auto loginLength = strlen( login );

        auto change = new Change{};
        change->type = type;

        if( password == nullptr ) {
            change->data = new char[groupLength+1+channelLength+1+loginLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = loginLength;
            memcpy( &change->data[groupLength+1+channelLength+1], login, loginLength );
        } else {
            change->data = new char[groupLength+1+channelLength+1+loginLength+1+crypto::HASH_LENGTH+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = loginLength;
            memcpy( &change->data[groupLength+1+channelLength+1], login, loginLength );
            change->values[3] = crypto::HASH_LENGTH;
            memcpy( &change->data[groupLength+1+channelLength+1+loginLength+1], password, crypto::HASH_LENGTH );
        }

        return change;
    }

    Changes::Change *Changes::addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        return _buildGroupChange( CH_CREATE_GROUP, group, password );
    }

    Changes::Change *Changes::updateGroupPassword( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        return _buildGroupChange( CH_UPDATE_GROUP_PASSWORD, group, password );
    }

    Changes::Change *Changes::removeGroup( const char *group ) {
        return _buildGroupChange( CH_REMOVE_GROUP, group, nullptr );
    }

    Changes::Change *Changes::addChannel(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        return _buildChannelChange( CH_CREATE_CHANNEL, group, channel, limitMessages );
    }

    Changes::Change *Changes::updateChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        return _buildChannelChange( CH_UPDATE_CHANNEL_SETTINGS, group, channel, limitMessages );
    }

    Changes::Change *Changes::removeChannel( const char *group, const char *channel ) {
        return _buildChannelChange( CH_REMOVE_CHANNEL, group, channel, nullptr );
    }
    
    Changes::Change *Changes::addConsumer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_CREATE_CONSUMER, group, channel, login, password );
    }

    Changes::Change *Changes::updateConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_UPDATE_CONSUMER_PASSWORD, group, channel, login, password );
    }

    Changes::Change *Changes::removeConsumer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        return _buildUserChange( CH_REMOVE_CONSUMER, group, channel, login, nullptr );
    }

    Changes::Change *Changes::addProducer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_CREATE_PRODUCER, group, channel, login, password );
    }

    Changes::Change *Changes::updateProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_UPDATE_PRODUCER_PASSWORD, group, channel, login, password );
    }

    Changes::Change *Changes::removeProducer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        return _buildUserChange( CH_REMOVE_PRODUCER, group, channel, login, nullptr );
    }

    Changes::Change *Changes::updatePort( unsigned short int port ) {
        if( !util::Validation::isPort( port ) ) {
            throw util::Error::WRONG_PARAM;
        }

        auto change = new Change{};
        change->type = CH_UPDATE_PORT;
        auto size = sizeof( unsigned short int );
        change->values[0] = size;

        change->data = new char[size+1]{};
        memcpy( change->data, &port, size );

        return change;
    }

    Changes::Change *Changes::updateCountThreads( unsigned short int threads ) {
        if( threads == 0 ) {
            throw util::Error::WRONG_PARAM;
        }

        auto change = new Change{};
        change->type = CH_UPDATE_COUNT_THREADS;
        auto size = sizeof( unsigned short int );
        change->values[0] = size;

        change->data = new char[size+1]{};
        memcpy( change->data, &threads, size );

        return change;
    }

    Changes::Change *Changes::updateMasterPassword(
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto change = new Change{};
        change->type = CH_UPDATE_COUNT_THREADS;
        change->values[0] = crypto::HASH_LENGTH;

        change->data = new char[crypto::HASH_LENGTH+1]{};
        memcpy( change->data, password, crypto::HASH_LENGTH );

        return change;
    }

    const char *Changes::getGroup( Change *change ) {
        return change->data;
    }

    const char *Changes::getChannel( Change *change ) {
        auto offset = change->values[0] + 1;
        return &change->data[offset];
    }

    const char *Changes::getLogin( Change *change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return &change->data[offset];
    }

    const util::types::ChannelLimitMessages *Changes::getChannelLimitMessages( Change *change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return (util::types::ChannelLimitMessages *)&change->data[offset];
    }

    unsigned short int Changes::getPort( Change *change ) {
        unsigned short int port;
        memcpy( &port, change->data, change->values[0] );
        return port;
    }

    unsigned short int Changes::getCountThreads( Change *change ) {
        unsigned short int count;
        memcpy( &count, change->data, change->values[0] );
        return count;
    }

    void Changes::getPassword( Change *change, unsigned char password[crypto::HASH_LENGTH] ) {
        unsigned int offset = 0;

        switch( change->type ) {
            case CH_UPDATE_MASTER_PASSWORD:
                offset = 0;
                break;
            case CH_CREATE_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
                offset = change->values[0]+1;
                break;
            case CH_CREATE_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_CREATE_PRODUCER:
            case CH_UPDATE_PRODUCER_PASSWORD:
                offset = change->values[0]+1;
                offset += change->values[1]+1;
                offset += change->values[2]+1;
                break;
            default:
                return;
        }

        memcpy( password, &change->data[offset], crypto::HASH_LENGTH );
    }

    bool Changes::isAddGroup( Change *change ) {
        return change->type == CH_CREATE_GROUP;
    }

    bool Changes::isUpdateGroupPassword( Change *change ) {
        return change->type == CH_UPDATE_GROUP_PASSWORD;
    }

    bool Changes::isRemoveGroup( Change *change ) {
        return change->type == CH_REMOVE_GROUP;
    }

    bool Changes::isAddChannel( Change *change ) {
        return change->type == CH_CREATE_CHANNEL;
    }

    bool Changes::isUpdateChannelLimitMessages( Change *change ) {
        return change->type == CH_UPDATE_CHANNEL_SETTINGS;
    }

    bool Changes::isRemoveChannel( Change *change ) {
        return change->type == CH_REMOVE_CHANNEL;
    }

    bool Changes::isAddConsumer( Change *change ) {
        return change->type == CH_CREATE_CONSUMER;
    }

    bool Changes::isUpdateConsumerPassword( Change *change ) {
        return change->type == CH_UPDATE_CONSUMER_PASSWORD;
    }

    bool Changes::isRemoveConsumer( Change *change ) {
        return change->type == CH_REMOVE_CONSUMER;
    }

    bool Changes::isAddProducer( Change *change ) {
        return change->type == CH_CREATE_PRODUCER;
    }

    bool Changes::isUpdateProducerPassword( Change *change ) {
        return change->type == CH_UPDATE_PRODUCER_PASSWORD;
    }

    bool Changes::isRemoveProducer( Change *change ) {
        return change->type == CH_REMOVE_PRODUCER;
    }

    bool Changes::isUpdatePort( Change *change ) {
        return change->type == CH_UPDATE_PORT;
    }
    bool Changes::isUpdateCountThreads( Change *change ) {
        return change->type == CH_UPDATE_COUNT_THREADS;
    }

    bool Changes::isUpdateMasterPassword( Change *change ) {
        return change->type == CH_UPDATE_MASTER_PASSWORD;
    }

    void Changes::free( Change *change ) {
        if( change->data != nullptr ) {
            delete[] change->data;
        }

        switch( change->initiator ) {
            case util::types::Initiator::I_GROUP:
                delete[] change->group;
                break;
            case util::types::Initiator::I_CONSUMER:
            case util::types::Initiator::I_PRODUCER:
                delete[] change->group;
                delete[] change->channel;
                delete[] change->login;
                break;
        }

        delete change;
    }

    void Changes::push(
        Change *change,
        util::types::Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto ch = new Change{};
        memcpy( ch, change, sizeof( Change ) );
        ch->initiator = initiator;

        switch( ch->initiator ) {
            case util::types::Initiator::I_GROUP:
                ch->group = util::String::copy( group );
                break;
            case util::types::Initiator::I_CONSUMER:
            case util::types::Initiator::I_PRODUCER:
                ch->group = util::String::copy( group );
                ch->channel = util::String::copy( channel );
                ch->login = util::String::copy( login );
                break;
        }

        auto length = 0;
        for( int i = 0; i < LENGTH_VALUES; i++ ) {
            length += change->values[i];
            if( change->values[i] ) {
                length += 1;
            }
        }

        ch->data = new char[length]{};
        memcpy( ch->data, change->data, length );

        listMemory.push_back( ch );
    }

    char *Changes::_convertChannelLimitMessagesDataToFile( Change *change ) {
        unsigned int length = 0;
        length += change->values[0] + 1;
        length += change->values[1] + 1;
        length += crypto::HASH_LENGTH + 1;

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        auto offset = change->values[0] + 1;
        offset += change->values[1] + 1;


        util::types::ChannelLimitMessages _limitMessages;
        memcpy( &_limitMessages, &data[offset], sizeof( util::types::ChannelLimitMessages ) );
        _limitMessages.minMessageSize = htonl( _limitMessages.minMessageSize );
        _limitMessages.maxMessageSize = htonl( _limitMessages.maxMessageSize );
        _limitMessages.maxMessagesOnDisk = htonl( _limitMessages.maxMessagesOnDisk );
        _limitMessages.maxMessagesInMemory = htonl( _limitMessages.maxMessagesInMemory );
        memcpy( &data[offset], &_limitMessages, sizeof( util::types::ChannelLimitMessages ) );

        return data;
    }

    char *Changes::_convertChannelLimitMessagesDataFromFile( Change *change ) {
        unsigned int length = 0;
        length += change->values[0] + 1;
        length += change->values[1] + 1;
        length += crypto::HASH_LENGTH + 1;

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        auto offset = change->values[0] + 1;
        offset += change->values[1] + 1;

        util::types::ChannelLimitMessages _limitMessages;
        memcpy( &_limitMessages, &data[offset], sizeof( util::types::ChannelLimitMessages ) );
        _limitMessages.minMessageSize = ntohl( _limitMessages.minMessageSize );
        _limitMessages.maxMessageSize = ntohl( _limitMessages.maxMessageSize );
        _limitMessages.maxMessagesOnDisk = ntohl( _limitMessages.maxMessagesOnDisk );
        _limitMessages.maxMessagesInMemory = ntohl( _limitMessages.maxMessagesInMemory );
        memcpy( &data[offset], &_limitMessages, sizeof( util::types::ChannelLimitMessages ) );

        return data;
    }

    char *Changes::_convertPortDataToFile( Change *change ) {
        unsigned int length = change->values[0] + 1;
        unsigned short int port;
        auto size = sizeof( unsigned short int );

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        memcpy( &port, data, size );
        port = htons( port );
        memcpy( data, &port, size );

        return data;
    }

    char *Changes::_convertPortDataFromFile( Change *change ) {
        unsigned int length = change->values[0] + 1;
        unsigned short int port;
        auto size = sizeof( unsigned short int );

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        memcpy( &port, data, size );
        port = ntohs( port );
        memcpy( data, &port, size );

        return data;
    }

    char *Changes::_convertCountThreadsDataToFile( Change *change ) {
        unsigned int length = change->values[0] + 1;
        unsigned short int count;
        auto size = sizeof( unsigned short int );

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        memcpy( &count, data, size );
        count = htons( count );
        memcpy( data, &count, size );

        return data;
    }

    char *Changes::_convertCountThreadsDataFromFile( Change *change ) {
        unsigned int length = change->values[0] + 1;
        unsigned short int count;
        auto size = sizeof( unsigned short int );

        auto data = new char[length]{};
        memcpy( data, change->data, length );

        memcpy( &count, data, size );
        count = ntohs( count );
        memcpy( data, &count, size );

        return data;
    }

    bool Changes::_isValidGroup( Change *change, unsigned int length ) {
        if( change->values[0] == 0 ) {
            return false;
        }

        auto l = change->values[0] + 1;

        if( change->type != CH_REMOVE_GROUP ) {
            l += change->values[1] + 1;
            if( change->values[1] != crypto::HASH_LENGTH ) {
                return false;
            }
        }

        if( l != length ) {
            return false;
        }

        return util::Validation::isGroupName( change->data );
    }

    bool Changes::_isValidChannel( Change *change, unsigned int length ) {
        if( change->values[0] == 0 || change->values[1] == 0 ) {
            return false;
        }

        auto l = change->values[0] + 1;
        l += change->values[1] + 1;
        auto lengthLimitMessages = sizeof( util::types::ChannelLimitMessages );

        if( change->type != CH_REMOVE_CHANNEL ) {
            l += change->values[2] + 1;

            if( change->values[2] != lengthLimitMessages ) {
                return false;
            }

            util::types::ChannelLimitMessages limitMessages;
            memcpy(
                &limitMessages,
                &change->data[change->values[0]+1+change->values[1]+1],
                lengthLimitMessages
            );

            if( !util::Validation::isChannelLimitMessages( limitMessages ) ) {
                return false;
            }
        }

        if( l != length ) {
            return false;
        }

        if( !util::Validation::isGroupName( change->data ) ) {
            return false;
        }

        return util::Validation::isChannelName( &change->data[change->values[0]+1] );
    }

    bool Changes::_isValidUser( Change *change, unsigned int length ) {
        if( change->values[0] == 0 || change->values[1] == 0 || change->values[2] == 0 ) {
            return false;
        }

        auto l = change->values[0] + 1;
        l += change->values[1] + 1;
        l += change->values[2] + 1;

        if( change->type != CH_REMOVE_CONSUMER && change->type != CH_REMOVE_PRODUCER ) {
            l += change->values[3] + 1;
            if( change->values[3] != crypto::HASH_LENGTH ) {
                return false;
            }
        }

        if( l != length ) {
            return false;
        }

        if( !util::Validation::isGroupName( change->data ) ) {
            return false;
        }

        if( !util::Validation::isChannelName( &change->data[change->values[0]+1] ) ) {
            return false;
        }

        switch( change->type ) {
            case CH_CREATE_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
                return util::Validation::isConsumerName(
                    &change->data[change->values[0]+1+change->values[1]+1]
                );
            default:
                return util::Validation::isProducerName(
                    &change->data[change->values[0]+1+change->values[1]+1]
                );
        }
    }

    bool Changes::_isValidUpdatePort( Change *change ) {
        return util::Validation::isPort( change->values[0] );
    }

    bool Changes::_isValidUpdateCountThreads( Change *change ) {
        return change->values[0] != 0;
    }

    bool Changes::_isValidUpdateMasterPassword( Change *change, unsigned int length ) {
        return change->values[0] != crypto::HASH_LENGTH || length != crypto::HASH_LENGTH + 1;
    }

    bool Changes::_isValidChangeFromFile( Change *change, unsigned int length ) {
        switch( change->type ) {
            case CH_CREATE_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
            case CH_REMOVE_GROUP:
                return _isValidGroup( change, length );
            case CH_CREATE_CHANNEL:
            case CH_UPDATE_CHANNEL_SETTINGS:
            case CH_REMOVE_CHANNEL:
                return _isValidChannel( change, length );
            case CH_CREATE_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
            case CH_CREATE_PRODUCER:
            case CH_UPDATE_PRODUCER_PASSWORD:
            case CH_REMOVE_PRODUCER:
                return _isValidUser( change, length );
            case CH_UPDATE_PORT:
                return _isValidUpdatePort( change );
            case CH_UPDATE_COUNT_THREADS:
                return _isValidUpdateCountThreads( change );
            case CH_UPDATE_MASTER_PASSWORD:
                return _isValidUpdateMasterPassword( change, length );
        }

        return false;
    }

    void Changes::pushDefered( Change *change ) {

        ChangeFile ch;

        ch.type = htonl( change->type );

        auto length = 0;

        for( unsigned int i = 0; i < LENGTH_VALUES; i++ ) {
            if( change->values[i] ) {
                length += change->values[i];
                // null byte
                length += 1;
            }
            ch.values[i] = htonl( change->values[i] );
        }

        char uuid[util::UUID::LENGTH+1];

        _generateUniqFileName( uuid );

        std::string tmpPath = _path;

        tmpPath += "_";
        tmpPath += uuid;

        auto file = util::File( tmpPath.c_str(), true );
        file.write( &ch, sizeof( ChangeFile ) );

        if( isUpdateChannelLimitMessages( change ) || isAddChannel( change ) ) {
            auto data = _convertChannelLimitMessagesDataToFile( change );
            file.write( data, length );
            delete[] data;
        } else if( isUpdatePort( change ) ) {
            auto data = _convertPortDataToFile( change );
            file.write( data, length );
            delete[] data;
        } else if( isUpdateCountThreads( change ) ) {
            auto data = _convertCountThreadsDataToFile( change );
            file.write( data, length );
            delete[] data;
        } else {
            file.write( change->data, length );
        }
        file.rename( uuid );
    }

    bool Changes::_isAllowedType( unsigned int type ) {
        switch( type ) {
            case CH_UPDATE_PORT:
            case CH_UPDATE_MASTER_PASSWORD:
            case CH_UPDATE_COUNT_THREADS:
            case CH_CREATE_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
            case CH_REMOVE_GROUP:
            case CH_CREATE_CHANNEL:
            case CH_UPDATE_CHANNEL_SETTINGS:
            case CH_REMOVE_CHANNEL:
            case CH_CREATE_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
            case CH_CREATE_PRODUCER:
            case CH_UPDATE_PRODUCER_PASSWORD:
            case CH_REMOVE_PRODUCER:
                return true;
            default:
                return false;
        }
    }

    void Changes::_generateUniqFileName( char *uuid ) {
        while( true ) {
            memset( uuid, 0, util::UUID::LENGTH+1 );
            util::UUID::generate( uuid );

            std::string _tmpDev = _path;
            _tmpDev += "_";
            _tmpDev += uuid;

            std::string _tmp = _path;
            _tmp += uuid;

            if( !util::FS::fileExists( _tmpDev.c_str() ) && !util::FS::fileExists( _tmp.c_str() ) ) {
                break;
            } 
        }
    }

    void Changes::_addChangesFromFile( util::File &file, const char *name ) {
        if( unknownFiles.find( name ) != unknownFiles.end() ) {
            return;
        }

        if( passedFiles.find( name ) != passedFiles.end() ) {
            return;
        }

        if( file.size() < sizeof( ChangeFile ) ) {
            return;
        }

        ChangeFile cf;

        file.read( &cf, sizeof( ChangeFile ) );

        cf.type = ntohl( cf.type );

        if( !_isAllowedType( cf.type ) ) {
            unknownFiles[name] = true;
            return;
        }

        Change *ch = new Change{};

        ch->type = cf.type;
        ch->initiator = util::types::Initiator::I_ROOT;

        for( int i = 0; i < LENGTH_VALUES; i++ ) {
            ch->values[i] = ntohl( cf.values[i] );
        }

        auto length = file.size() - sizeof( ChangeFile );
        if( length != 0 ) {
            ch->data = new char[length]{};
            file.read( ch->data, length );
        }

        if( !_isValidChangeFromFile( ch, length ) ) {
            failedFiles.push_back( name );
            free( ch );
            return;
        }

        if( isUpdateChannelLimitMessages( ch ) || isAddChannel( ch ) ) {
            auto _tmpData = ch->data;
            ch->data = _convertChannelLimitMessagesDataFromFile( ch );
            delete[] _tmpData;
        } else if( isUpdatePort( ch ) ) {
            auto _tmpData = ch->data;
            ch->data = _convertPortDataFromFile( ch );
            delete[] _tmpData;
        } else if( isUpdateCountThreads( ch ) ) {
            auto _tmpData = ch->data;
            ch->data = _convertCountThreadsDataFromFile( ch );
            delete[] _tmpData;
        }

        passedFiles[name] = true;


        ChangeDisk cd;
        cd.change = ch;
        memcpy( cd.uuid, name, util::UUID::LENGTH );

        listDisk.push_back( cd );
    }

    void Changes::_addChangesFromFiles() {
        std::vector<std::string> files;

        util::FS::files( _path, files, util::FS::SortAsc, util::FS::SortByDate );

        for( auto it = files.begin(); it != files.end(); it++ ) {
            if( !util::Validation::isUUID( (*it).c_str() ) ) {
                continue;
            }

            std::string p = _path;
            p += *it;

            util::File file( p.c_str() );

            _addChangesFromFile( file, (*it).c_str() );
        }

    }

    void Changes::_popFile() {
        std::string p = _path;

        auto name = listDisk.front().uuid;

        auto fileName = std::string( name, util::UUID::LENGTH );

        p += fileName;

        util::FS::removeFile( p.c_str() );

        auto it = passedFiles.find( name );

        if( it != passedFiles.end() ) {
            passedFiles.erase( it );
        }
    }

    Changes::Change *Changes::pop() {
        std::lock_guard<std::mutex> lock( m );
        _clearFailedFiles();

        Change *change = nullptr;

        if( listDisk.empty() ) {

            _addChangesFromFiles();
        }


        if( !listDisk.empty() ) {
            change = listDisk.front().change;

            _popFile();
            listDisk.pop_front();

            return change;
        } else if( !listMemory.empty() ) {
            change = listMemory.front();

            listMemory.pop_front();

            return change;

        }

        return nullptr;
    }

    util::types::Initiator Changes::getInitiator( Change *change ) {
        return change->initiator;
    }

    const char *Changes::getInitiatorGroup( Change *change ) {
        return change->group;
    }

    const char *Changes::getInitiatorChannel( Change *change ) {
        return change->channel;
    }

    const char *Changes::getInitiatorLogin( Change *change ) {
        return change->login;
    }
}

#endif
