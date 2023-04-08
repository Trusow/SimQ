#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/file.hpp"
#include "../../util/fs.hpp"
#include "../../util/validation.hpp"
#include "../../util/uuid.hpp"
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
            const char *defPath = "changes";
            const unsigned int LENGTH_VALUES = 16;

            struct ChangeFile {
                unsigned int type;
                unsigned int values[16];
            };

        public:

            struct Change: ChangeFile {
                char *data;
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
                util::Types::ChannelSettings *settings
            );

            Change *_buildUserChange(
                unsigned int type,
                const char *group,
                const char *channel,
                const char *login,
                unsigned char password[crypto::HASH_LENGTH]
            );

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

            Change *addChannel( const char *group, const char *channel );
            Change *updateChannelSettings(
                const char *group,
                const char *channel,
                util::Types::ChannelSettings *settings
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
            bool isRemoveGroup( Change *change );

            const char *getGroup( Change *change );
            const char *getChannel( Change *change );
            const char *getLogin( Change *change );
            const util::Types::ChannelSettings *getChannelSettings( Change *change );
            unsigned int getPort( Change *change );
            unsigned int getCountThreads( Change *change );
            void getPassword( Change *change, unsigned char password[crypto::HASH_LENGTH] );

            void push( Change *change );
            void pushDefered( Change *change );
            Change *pop();

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
        util::Types::ChannelSettings *settings
    ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        if( !util::Validation::isChannelName( channel ) ) {
            throw util::Error::WRONG_CHANNEL;
        }

        if( settings != nullptr ) {
            unsigned long int _size = settings->maxMessagesOnDisk + settings->maxMessagesInMemory;

            if( _size > 0xFF'FF'FF'FF ) {
                throw util::Error::WRONG_SETTINGS;
            }

            if( settings->minMessageSize > settings->maxMessageSize ) {
                throw util::Error::WRONG_SETTINGS;
            }
        }

        auto groupLength = strlen( group );
        auto channelLength = strlen( channel );
        auto settingsLength = sizeof( util::Types::ChannelSettings );

        auto change = new Change{};
        change->type = type;

        if( settings != nullptr ) {
            change->data = new char[groupLength+1+channelLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
        } else {
            change->data = new char[groupLength+1+channelLength+1+settingsLength]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = settingsLength;
            memcpy( &change->data[groupLength+1+channelLength+1], settings, settingsLength );
        }

        return change;
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

        if( password != nullptr ) {
            change->data = new char[groupLength+1+channelLength+1+loginLength+1]{};

            change->values[0] = groupLength;
            memcpy( change->data, group, groupLength );
            change->values[1] = channelLength;
            memcpy( &change->data[groupLength+1], channel, channelLength );
            change->values[2] = loginLength;
            memcpy( &change->data[groupLength+1+channelLength+1], login, loginLength );
        } else {
            change->data = new char[groupLength+1+channelLength+1+loginLength+1+crypto::HASH_LENGTH]{};

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

    Changes::Change *Changes::addChannel( const char *group, const char *channel ) {
        return _buildChannelChange( CH_CREATE_CHANNEL, group, channel, nullptr );
    }

    Changes::Change *Changes::updateChannelSettings(
        const char *group,
        const char *channel,
        util::Types::ChannelSettings *settings
    ) {
        return _buildChannelChange( CH_UPDATE_CHANNEL_SETTINGS, group, channel, settings );
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
        if( port == 0 ) {
            throw util::Error::WRONG_PARAM;
        }

        auto change = new Change{};
        change->type = CH_UPDATE_PORT;
        change->values[0] = port;

        return change;
    }

    Changes::Change *Changes::updateCountThreads( unsigned short int threads ) {
        if( threads == 0 ) {
            throw util::Error::WRONG_PARAM;
        }

        auto change = new Change{};
        change->type = CH_UPDATE_COUNT_THREADS;
        change->values[0] = threads;

        return change;
    }

    Changes::Change *Changes::updateMasterPassword(
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        auto change = new Change{};
        change->type = CH_UPDATE_COUNT_THREADS;
        change->values[0] = crypto::HASH_LENGTH;

        change->data = new char[crypto::HASH_LENGTH]{};
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

    const util::Types::ChannelSettings *Changes::getChannelSettings( Change *change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return (util::Types::ChannelSettings *)&change->data[offset];
    }

    unsigned int Changes::getPort( Change *change ) {
        return change->values[0];
    }

    unsigned int Changes::getCountThreads( Change *change ) {
        return change->values[0];
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

    bool Changes::isRemoveGroup( Change *change ) {
        return change->type == CH_REMOVE_GROUP;
    }

    void Changes::free( Change *change ) {
        if( change->data != nullptr ) {
            delete[] change->data;
        }
        delete change;
    }

    void Changes::push( Change *change ) {
        std::lock_guard<std::mutex> lock( m );

        listMemory.push_back( change );
    }

    void Changes::pushDefered( Change *change ) {

        ChangeFile ch;

        ch.type = htonl( change->type );

        auto length = 0;

        for( unsigned int i = 0; i < LENGTH_VALUES; i++ ) {
            length += change->values[i];
            if( change->values[i] ) {
                // null byte
                length += 1;
            }
            ch.values[i] = htonl( change->values[i] );
        }

        // TODO: Доработать хранение числовых данных в дате

        char uuid[util::UUID::LENGTH+1];

        _generateUniqFileName( uuid );

        std::string tmpPath = _path;

        tmpPath += "_";
        tmpPath += uuid;

        auto file = util::File( tmpPath.c_str(), true );
        file.write( &ch, sizeof( ChangeFile ) );
        file.write( change->data, length );
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

        if( _isAllowedType( cf.type ) ) {
            passedFiles[name] = true;
        } else {
            unknownFiles[name] = true;
            return;
        }

        Change *ch = new Change{};

        ch->type = cf.type;
        for( int i = 0; i < LENGTH_VALUES; i++ ) {
            ch->values[i] = ntohl( cf.values[i] );
        }

        auto length = file.size() - sizeof( ChangeFile );
        ch->data = new char[length]{};
        file.read( ch->data, length );

        ChangeDisk cd;
        cd.change = ch;
        memcpy( cd.uuid, name, util::UUID::LENGTH );

        listDisk.push_back( cd );
    }

    void Changes::_addChangesFromFiles() {
        std::vector<std::string> files;

        // TODO: получать файлы отсортированные по дате
        util::FS::files( _path, files );

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

        Change *change = nullptr;

        if( listDisk.empty() ) {

            _addChangesFromFiles();

            if( listDisk.empty() ) {
                return nullptr;
            }

            change = listDisk.front().change;

            _popFile();
            listDisk.pop_front();
        } else {
            if( listMemory.empty() ) {
                return nullptr;
            }

            change = listMemory.front();
            listMemory.pop_front();
        }

        return change;
    }
}

#endif
