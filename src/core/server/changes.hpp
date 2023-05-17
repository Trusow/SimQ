#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/file.hpp"
#include "../../util/fs.hpp"
#include "../../util/validation.hpp"
#include "../../util/uuid.hpp"
#include "../../util/constants.h"
#include <list>
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <arpa/inet.h>
#include <memory>

namespace simq::core::server {
    class Changes {
        private:
            enum Type {
                CH_ADD_GROUP = 1000,
                CH_UPDATE_GROUP_PASSWORD,
                CH_REMOVE_GROUP,

                CH_ADD_CHANNEL = 2000,
                CH_UPDATE_CHANNEL_LIMIT_MESSAGES,
                CH_REMOVE_CHANNEL,

                CH_ADD_CONSUMER = 3000,
                CH_UPDATE_CONSUMER_PASSWORD,
                CH_REMOVE_CONSUMER,

                CH_ADD_PRODUCER = 4000,
                CH_UPDATE_PRODUCER_PASSWORD,
                CH_REMOVE_PRODUCER,
            };
            const char *defPath = util::constants::PATH_DIR_CHANGES;
            const unsigned int LENGTH_VALUES = 16;

            struct ChangeFile {
                unsigned int type;
                unsigned int values[16];
            };

        public:

            struct Change: ChangeFile {
                std::unique_ptr<char[]> data;
                unsigned int ip;
                util::types::Initiator initiator;
                std::string group;
                std::string channel;
                std::string login;
            };

        private:

            std::unique_ptr<char[]> _path;

            struct ChangeDisk {
                std::unique_ptr<Change> change;
                char uuid[util::UUID::LENGTH];
            };

            std::list<std::unique_ptr<Change>> listMemory;
            std::list<std::unique_ptr<ChangeDisk>> listDisk;
            std::map<std::string, bool> unknownFiles;
            std::map<std::string, bool> passedFiles;
            std::list<std::string> failedFiles;
            std::mutex m;

            void _addChangesFromFiles();
            void _addChangesFromFile( util::File &file, const char *name );
            void _popFile();
            void _generateUniqFileName( char *name );
            bool _isAllowedType( unsigned int type );
            std::unique_ptr<Change> _buildGroupChange(
                unsigned int type,
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );

            std::unique_ptr<Change> _buildChannelChange(
                unsigned int type,
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );

            std::unique_ptr<Change> _buildUserChange(
                unsigned int type,
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );

            std::unique_ptr<char[]> _convertChannelLimitMessagesDataToFile( Change *change );
            std::unique_ptr<char[]> _convertChannelLimitMessagesDataFromFile( Change *change );

            bool _isValidGroup( Change *change, unsigned int length );
            bool _isValidChannel( Change *change, unsigned int length );
            bool _isValidUser( Change *change, unsigned int length );
            bool _isValidChangeFromFile( Change *change, unsigned int length );

            void _clearFailedFiles();

        public:
            Changes( const char *path );

            std::unique_ptr<Change> addGroup(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> updateGroupPassword(
                const char *group,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> removeGroup( const char *group );

            std::unique_ptr<Change> addChannel(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            std::unique_ptr<Change> updateChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages *limitMessages
            );
            std::unique_ptr<Change> removeChannel( const char *group, const char *channel );

            std::unique_ptr<Change> addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> removeConsumer(
                const char *group,
                const char *channel,
                const char *login
            );

            std::unique_ptr<Change> addProducer(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );
            std::unique_ptr<Change> removeProducer(
                const char *group,
                const char *channel,
                const char *login
            );

            bool isAddGroup( std::unique_ptr<Change> &change );
            bool isUpdateGroupPassword( std::unique_ptr<Change> &change );
            bool isRemoveGroup( std::unique_ptr<Change> &change );

            bool isAddChannel( std::unique_ptr<Change> &change );
            bool isUpdateChannelLimitMessages( std::unique_ptr<Change> &change );
            bool isRemoveChannel( std::unique_ptr<Change> &change );

            bool isAddConsumer( std::unique_ptr<Change> &change );
            bool isUpdateConsumerPassword( std::unique_ptr<Change> &change );
            bool isRemoveConsumer( std::unique_ptr<Change> &change );

            bool isAddProducer( std::unique_ptr<Change> &change );
            bool isUpdateProducerPassword( std::unique_ptr<Change> &change );
            bool isRemoveProducer( std::unique_ptr<Change> &change );

            const char *getGroup( std::unique_ptr<Change> &change );
            const char *getChannel( std::unique_ptr<Change> &change );
            const char *getLogin( std::unique_ptr<Change> &change );
            const util::types::ChannelLimitMessages *getChannelLimitMessages( std::unique_ptr<Change> &change );
            void getPassword( std::unique_ptr<Change> &change, unsigned char password[crypto::HASH_LENGTH] );

            void push(
                std::unique_ptr<Change> change,
                unsigned int ip = 0,
                util::types::Initiator initiator = util::types::Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
            void pushDefered( std::unique_ptr<Change> change );
            std::unique_ptr<Change> pop();

            util::types::Initiator getInitiator( std::unique_ptr<Change> &change );
            unsigned int getIP( std::unique_ptr<Change> &change );
            const char *getInitiatorGroup( std::unique_ptr<Change> &change );
            const char *getInitiatorChannel( std::unique_ptr<Change> &change );
            const char *getInitiatorLogin( std::unique_ptr<Change> &change );
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

        _path = std::make_unique<char[]>( l+1 );
        memcpy( _path.get(), _strPath, l );
    }

    std::unique_ptr<Changes::Change> Changes::_buildGroupChange(
        unsigned int type,
        const char *group,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        auto groupLength = strlen( group );


        auto change = std::make_unique<Change>();
        change->type = type;

        if( password != nullptr ) {
            change->data = std::make_unique<char[]>( groupLength+1+crypto::HASH_LENGTH );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
            change->values[1] = crypto::HASH_LENGTH;
            memcpy( &change->data[groupLength+1], password, crypto::HASH_LENGTH );
        } else {
            change->data = std::make_unique<char[]>(groupLength+1 );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
        }

        return change;
    }

    std::unique_ptr<Changes::Change> Changes::_buildChannelChange(
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

        auto change = std::make_unique<Change>();
        change->type = type;

        if( limitMessages == nullptr ) {
            change->data = std::make_unique<char[]>( groupLength+1+channelLength+1 );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data.get()[groupLength+1], channel, channelLength );
        } else {
            change->data = std::make_unique<char[]>( groupLength+1+channelLength+1+limitMessagesLength+1 );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data.get()[groupLength+1], channel, channelLength );
            change->values[2] = limitMessagesLength;
            memcpy( &change->data.get()[groupLength+1+channelLength+1], limitMessages, limitMessagesLength );
        }

        return change;
    }

    void Changes::_clearFailedFiles() {
        for( auto it = failedFiles.begin(); it != failedFiles.end(); it++ ) {
            std::string p = _path.get();
            p += *it;

            util::FS::removeFile( p.c_str() );
        }

        failedFiles.clear();
    }

    std::unique_ptr<Changes::Change> Changes::_buildUserChange(
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
            case CH_ADD_CONSUMER:
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

        auto change = std::make_unique<Change>();
        change->type = type;

        if( password == nullptr ) {
            change->data = std::make_unique<char[]>( groupLength+1+channelLength+1+loginLength+1 );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = loginLength;
            memcpy( &change->data[groupLength+1+channelLength+1], login, loginLength );
        } else {
            change->data = std::make_unique<char[]>( groupLength+1+channelLength+1+loginLength+1+crypto::HASH_LENGTH+1 );

            change->values[0] = groupLength;
            memcpy( change->data.get(), group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = loginLength;
            memcpy( &change->data[groupLength+1+channelLength+1], login, loginLength );
            change->values[3] = crypto::HASH_LENGTH;
            memcpy( &change->data[groupLength+1+channelLength+1+loginLength+1], password, crypto::HASH_LENGTH );
        }

        return change;
    }

    std::unique_ptr<Changes::Change> Changes::addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        return _buildGroupChange( CH_ADD_GROUP, group, password );
    }

    std::unique_ptr<Changes::Change> Changes::updateGroupPassword( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        return _buildGroupChange( CH_UPDATE_GROUP_PASSWORD, group, password );
    }

    std::unique_ptr<Changes::Change> Changes::removeGroup( const char *group ) {
        return _buildGroupChange( CH_REMOVE_GROUP, group, nullptr );
    }

    std::unique_ptr<Changes::Change> Changes::addChannel(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        return _buildChannelChange( CH_ADD_CHANNEL, group, channel, limitMessages );
    }

    std::unique_ptr<Changes::Change> Changes::updateChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages *limitMessages
    ) {
        return _buildChannelChange( CH_UPDATE_CHANNEL_LIMIT_MESSAGES, group, channel, limitMessages );
    }

    std::unique_ptr<Changes::Change> Changes::removeChannel( const char *group, const char *channel ) {
        return _buildChannelChange( CH_REMOVE_CHANNEL, group, channel, nullptr );
    }
    
    std::unique_ptr<Changes::Change> Changes::addConsumer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_ADD_CONSUMER, group, channel, login, password );
    }

    std::unique_ptr<Changes::Change> Changes::updateConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_UPDATE_CONSUMER_PASSWORD, group, channel, login, password );
    }

    std::unique_ptr<Changes::Change> Changes::removeConsumer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        return _buildUserChange( CH_REMOVE_CONSUMER, group, channel, login, nullptr );
    }

    std::unique_ptr<Changes::Change> Changes::addProducer(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_ADD_PRODUCER, group, channel, login, password );
    }

    std::unique_ptr<Changes::Change> Changes::updateProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        return _buildUserChange( CH_UPDATE_PRODUCER_PASSWORD, group, channel, login, password );
    }

    std::unique_ptr<Changes::Change> Changes::removeProducer(
        const char *group,
        const char *channel,
        const char *login
    ) {
        return _buildUserChange( CH_REMOVE_PRODUCER, group, channel, login, nullptr );
    }

    const char *Changes::getGroup( std::unique_ptr<Change> &change ) {
        return change->data.get();
    }

    const char *Changes::getChannel( std::unique_ptr<Change> &change ) {
        auto offset = change->values[0] + 1;
        return &change->data[offset];
    }

    const char *Changes::getLogin( std::unique_ptr<Change> &change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return &change->data[offset];
    }

    const util::types::ChannelLimitMessages *Changes::getChannelLimitMessages( std::unique_ptr<Change> &change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return (util::types::ChannelLimitMessages *)&change->data[offset];
    }

    void Changes::getPassword( std::unique_ptr<Change> &change, unsigned char password[crypto::HASH_LENGTH] ) {
        unsigned int offset = 0;

        switch( change->type ) {
            case CH_ADD_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
                offset = change->values[0]+1;
                break;
            case CH_ADD_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_ADD_PRODUCER:
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

    bool Changes::isAddGroup( std::unique_ptr<Change> &change ) {
        return change->type == CH_ADD_GROUP;
    }

    bool Changes::isUpdateGroupPassword( std::unique_ptr<Change> &change ) {
        return change->type == CH_UPDATE_GROUP_PASSWORD;
    }

    bool Changes::isRemoveGroup( std::unique_ptr<Change> &change ) {
        return change->type == CH_REMOVE_GROUP;
    }

    bool Changes::isAddChannel( std::unique_ptr<Change> &change ) {
        return change->type == CH_ADD_CHANNEL;
    }

    bool Changes::isUpdateChannelLimitMessages( std::unique_ptr<Change> &change ) {
        return change->type == CH_UPDATE_CHANNEL_LIMIT_MESSAGES;
    }

    bool Changes::isRemoveChannel( std::unique_ptr<Change> &change ) {
        return change->type == CH_REMOVE_CHANNEL;
    }

    bool Changes::isAddConsumer( std::unique_ptr<Change> &change ) {
        return change->type == CH_ADD_CONSUMER;
    }

    bool Changes::isUpdateConsumerPassword( std::unique_ptr<Change> &change ) {
        return change->type == CH_UPDATE_CONSUMER_PASSWORD;
    }

    bool Changes::isRemoveConsumer( std::unique_ptr<Change> &change ) {
        return change->type == CH_REMOVE_CONSUMER;
    }

    bool Changes::isAddProducer( std::unique_ptr<Change> &change ) {
        return change->type == CH_ADD_PRODUCER;
    }

    bool Changes::isUpdateProducerPassword( std::unique_ptr<Change> &change ) {
        return change->type == CH_UPDATE_PRODUCER_PASSWORD;
    }

    bool Changes::isRemoveProducer( std::unique_ptr<Change> &change ) {
        return change->type == CH_REMOVE_PRODUCER;
    }

    void Changes::push(
        std::unique_ptr<Change> change,
        unsigned int ip,
        util::types::Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::lock_guard<std::mutex> lock( m );

        change->ip = ip;
        change->initiator = initiator;

        switch( change->initiator ) {
            case util::types::Initiator::I_GROUP:
                change->group = group;
                break;
            case util::types::Initiator::I_CONSUMER:
            case util::types::Initiator::I_PRODUCER:
                change->group = group;
                change->channel = channel;
                change->login = login;
                break;
        }

        auto length = 0;
        for( int i = 0; i < LENGTH_VALUES; i++ ) {
            length += change->values[i];
            if( change->values[i] ) {
                length += 1;
            }
        }

        listMemory.push_back( std::move( change ) );
    }

    std::unique_ptr<char[]> Changes::_convertChannelLimitMessagesDataToFile( Change *change ) {
        unsigned int length = 0;
        length += change->values[0] + 1;
        length += change->values[1] + 1;
        length += crypto::HASH_LENGTH + 1;

        auto data = std::make_unique<char[]>(length);
        memcpy( data.get(), change->data.get(), length );

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

    std::unique_ptr<char[]> Changes::_convertChannelLimitMessagesDataFromFile( Change *change ) {
        unsigned int length = 0;
        length += change->values[0] + 1;
        length += change->values[1] + 1;
        length += crypto::HASH_LENGTH + 1;

        auto data = std::make_unique<char[]>(length);
        memcpy( data.get(), change->data.get(), length );

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

        return util::Validation::isGroupName( change->data.get() );
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

        if( !util::Validation::isGroupName( change->data.get() ) ) {
            return false;
        }

        return util::Validation::isChannelName( &change->data.get()[change->values[0]+1] );
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

        if( !util::Validation::isGroupName( change->data.get() ) ) {
            return false;
        }

        if( !util::Validation::isChannelName( &change->data[change->values[0]+1] ) ) {
            return false;
        }

        switch( change->type ) {
            case CH_ADD_CONSUMER:
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

    bool Changes::_isValidChangeFromFile( Change *change, unsigned int length ) {
        switch( change->type ) {
            case CH_ADD_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
            case CH_REMOVE_GROUP:
                return _isValidGroup( change, length );
            case CH_ADD_CHANNEL:
            case CH_UPDATE_CHANNEL_LIMIT_MESSAGES:
            case CH_REMOVE_CHANNEL:
                return _isValidChannel( change, length );
            case CH_ADD_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
            case CH_ADD_PRODUCER:
            case CH_UPDATE_PRODUCER_PASSWORD:
            case CH_REMOVE_PRODUCER:
                return _isValidUser( change, length );
        }

        return false;
    }

    void Changes::pushDefered( std::unique_ptr<Change> change ) {

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

        std::string tmpPath = _path.get();

        tmpPath += "_";
        tmpPath += uuid;

        util::File file( tmpPath.c_str(), true );
        file.write( &ch, sizeof( ChangeFile ) );

        if( isUpdateChannelLimitMessages( change ) || isAddChannel( change ) ) {
            auto data = _convertChannelLimitMessagesDataToFile( change.get() );
            file.write( data.get(), length );
        } else {
            file.write( change->data.get(), length );
        }
        file.rename( uuid );
    }

    bool Changes::_isAllowedType( unsigned int type ) {
        switch( type ) {
            case CH_ADD_GROUP:
            case CH_UPDATE_GROUP_PASSWORD:
            case CH_REMOVE_GROUP:
            case CH_ADD_CHANNEL:
            case CH_UPDATE_CHANNEL_LIMIT_MESSAGES:
            case CH_REMOVE_CHANNEL:
            case CH_ADD_CONSUMER:
            case CH_UPDATE_CONSUMER_PASSWORD:
            case CH_REMOVE_CONSUMER:
            case CH_ADD_PRODUCER:
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

            std::string _tmpDev = _path.get();
            _tmpDev += "_";
            _tmpDev += uuid;

            std::string _tmp = _path.get();
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

        auto ch = std::make_unique<Change>();

        ch->type = cf.type;
        ch->initiator = util::types::Initiator::I_ROOT;

        for( int i = 0; i < LENGTH_VALUES; i++ ) {
            ch->values[i] = ntohl( cf.values[i] );
        }

        auto length = file.size() - sizeof( ChangeFile );
        if( length != 0 ) {
            ch->data = std::make_unique<char[]>( length );
            file.read( ch->data.get(), length );
        }

        if( !_isValidChangeFromFile( ch.get(), length ) ) {
            failedFiles.push_back( name );
            return;
        }

        if( isUpdateChannelLimitMessages( ch ) || isAddChannel( ch ) ) {
            ch->data = _convertChannelLimitMessagesDataFromFile( ch.get() );
        }

        passedFiles[name] = true;


        auto cd = std::make_unique<ChangeDisk>();
        cd->change = std::move( ch );
        memcpy( cd->uuid, name, util::UUID::LENGTH );

        listDisk.push_back( std::move( cd ) );
    }

    void Changes::_addChangesFromFiles() {
        std::vector<std::string> files;

        util::FS::files( _path.get(), files, util::FS::SortAsc, util::FS::SortByDate );

        for( auto it = files.begin(); it != files.end(); it++ ) {
            if( !util::Validation::isUUID( (*it).c_str() ) ) {
                continue;
            }

            std::string p = _path.get();
            p += *it;

            util::File file( p.c_str() );

            _addChangesFromFile( file, (*it).c_str() );
        }

    }

    void Changes::_popFile() {
        std::string p = _path.get();

        auto name = listDisk.front()->uuid;

        auto fileName = std::string( name, util::UUID::LENGTH );

        p += fileName;

        util::FS::removeFile( p.c_str() );

        passedFiles.erase( name );
    }

    std::unique_ptr<Changes::Change> Changes::pop() {
        std::lock_guard<std::mutex> lock( m );
        _clearFailedFiles();

        if( listDisk.empty() ) {

            _addChangesFromFiles();
        }


        if( !listDisk.empty() ) {
            auto change = std::move(listDisk.front()->change);

            _popFile();
            listDisk.pop_front();

            return change;
        } else if( !listMemory.empty() ) {
            auto change = std::move(listMemory.front());

            listMemory.pop_front();

            return change;

        }

        return nullptr;
    }

    util::types::Initiator Changes::getInitiator( std::unique_ptr<Change> &change ) {
        return change->initiator;
    }

    unsigned int Changes::getIP( std::unique_ptr<Change> &change ) {
        return change->ip;
    }

    const char *Changes::getInitiatorGroup( std::unique_ptr<Change> &change ) {
        return change->group.c_str();
    }

    const char *Changes::getInitiatorChannel( std::unique_ptr<Change> &change ) {
        return change->channel.c_str();
    }

    const char *Changes::getInitiatorLogin( std::unique_ptr<Change> &change ) {
        return change->login.c_str();
    }
}

#endif
