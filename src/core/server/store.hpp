#ifndef SIMQ_CORE_SERVER_STORE 
#define SIMQ_CORE_SERVER_STORE 

#include <string>
#include <string.h>
#include <mutex>
#include <vector>
#include <list>
#include <thread>
#include <arpa/inet.h>
#include <map>
#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/validation.hpp"
#include "../../util/fs.hpp"
#include "../../util/file.hpp"
#include "../../util/error.h"
#include "../../util/constants.h"


namespace simq::core::server {
    class Store {
        private:
            const unsigned int DEFAULT_PORT = util::constants::DEFAULT_PORT;

            struct Settings {
                unsigned short int countThreads;
                unsigned short int port;
                unsigned char password[crypto::HASH_LENGTH];
            };

            std::mutex m;

            struct Channel {
                util::types::ChannelLimitMessages limitMessages;
                std::map<std::string, bool> consumers;
                std::map<std::string, bool> producers;
            };

            std::map<std::string, std::map<std::string, Channel>> groups;

            enum TypeUser {
                CONSUMER,
                PRODUCER,
            };

            std::unique_ptr<char[]> _path;

            void _initSettings();
            void _initChannels( const char *group );
            void _initChannelLimitMessages( const char *group, const char *channel );
            void _initUsers( const char *group, const char *channel, TypeUser type );
            void _initGroups();
            bool _issetCorrectPasswordFile( const char *path );
            void _getPassword( const char *path, unsigned char *password );
        public:
            Store( const char *path );

            void getDirectGroups( std::vector<std::string> &list );
            void getDirectChannels( const char *group, std::vector<std::string> &list );
            void getDirectChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages &limitMessages
            );
            void getDirectConsumers( const char *group, const char *channel, std::vector<std::string> &list );
            void getDirectProducers( const char *group, const char *channel, std::vector<std::string> &list );
            unsigned short int getDirectPort();
            unsigned short int getDirectCountThreads();
            void getDirectMasterPassword(
                unsigned char *password
            );

            void getGroups( std::vector<std::string> &groups );
            void getGroupPassword( const char *group, unsigned char *password );
            void addGroup( const char *group, const unsigned char *password );
            void updateGroupPassword( const char *group, const unsigned char *password );
            void removeGroup( const char *group );

            void getChannels( const char *group, std::list<std::string> &channels );
            void getChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages &limitMessages
            );
            void addChannel(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages &limitMessages
            );
            void updateChannelLimitMessages(
                const char *group,
                const char *channel,
                util::types::ChannelLimitMessages limitMessages
            );
            void removeChannel( const char *group, const char *channel );

            void getConsumers( const char *group, const char *channel, std::list<std::string> &consumers );
            void addConsumer(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void getConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char *password
            );
            void updateConsumerPassword(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void removeConsumer( const char *group, const char *channel, const char *login );

            void getProducers( const char *group, const char *channel, std::list<std::string> &producers );
            void addProducer(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void getProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                unsigned char *password
            );
            void updateProducerPassword(
                const char *group,
                const char *channel,
                const char *login,
                const unsigned char *password
            );
            void removeProducer( const char *group, const char *channel, const char *login );

            void getMasterPassword( unsigned char *password );
            void updateMasterPassword( const unsigned char *password );

            unsigned short int getCountThreads();
            void updateCountThreads( unsigned short int count );

            unsigned short int getPort();
            void updatePort( unsigned short int port );
    };

    Store::Store( const char *path ) {
        auto length = strlen( path );
        _path = std::make_unique<char[]>( length + 1 );
        memcpy( _path.get(), path, length );

        _initSettings();
        _initGroups();
    }

    void Store::_initSettings() {
        Settings settings;

        bool isNewSettings = false;
        const unsigned int hc = std::thread::hardware_concurrency();

        std::string pathToDir;
        util::constants::buildPathToDirSettings( pathToDir, _path.get() );
        const char *_pathToDir = pathToDir.c_str();

        std::string pathToFile;
        util::constants::buildPathToSettings( pathToFile, _path.get() );
        const char *_pathToFile = pathToFile.c_str();

        if( !util::FS::dirExists( _pathToDir ) ) {
            if( !util::FS::createDir( _pathToDir ) ) {
                throw util::Error::FS_ERROR;
            }
            auto _f = util::File( _pathToFile, true );
            isNewSettings = true;
        } else if( !util::FS::fileExists( _pathToFile ) ) {
            auto _f = util::File( _pathToFile, true );
            isNewSettings = true;
        }

        auto file = util::File( _pathToFile );

        if( file.size() < sizeof( Settings ) ) {
            isNewSettings = true;
        }

        if( isNewSettings ) {
            settings.countThreads = htons( hc );
            settings.port = htons( DEFAULT_PORT );
            crypto::Hash::hash( "simq", settings.password );

            file.write( &settings, sizeof( Settings ), 0 );
        } else {
            file.read( &settings, sizeof( Settings ), 0 );

            settings.countThreads = ntohs( settings.countThreads );
            settings.port = ntohs( settings.port );

            bool isWrong = false;

            if( settings.countThreads > hc + hc / 2 ) {
                settings.countThreads = htons( hc + hc / 2 );
                isWrong = true;
            } else if( settings.countThreads == 0 ) {
                settings.countThreads = htons( 1 );
                isWrong = true;
            }

            if( settings.port > 65535 || settings.port == 0 ) {
                isWrong = true;
                settings.port = htons( DEFAULT_PORT );
            }

            if( isWrong ) {
                file.write( &settings, sizeof( Settings ), 0 );
            }
        }


    }

    bool Store::_issetCorrectPasswordFile( const char *path ) {
        if( !util::FS::fileExists( path ) ) {
            return false;
        }

        util::File file( path );
        if( file.size() != crypto::HASH_LENGTH ) {
            return false;
        }
        
        return true;
    }

    void Store::_initUsers( const char *group, const char *channel, TypeUser type ) {
        std::vector<std::string> dirs;

        std::string path;

        if( type == TypeUser::CONSUMER ) {
            util::constants::buildPathToConsumers( path, _path.get(), group, channel );
        } else if( type == TypeUser::PRODUCER ) {
            util::constants::buildPathToProducers( path, _path.get(), group, channel );
        }

        util::FS::dirs( path.c_str(), dirs );

        for( auto it = dirs.begin(); it != dirs.end(); it++ ) {
            auto login = (*it).c_str();
            std::string pathPassword;

            if( type == TypeUser::CONSUMER ) {
                util::constants::buildPathToConsumerPassword( pathPassword, _path.get(), group, channel, login );
            } else if( type == TypeUser::PRODUCER ) {
                util::constants::buildPathToProducerPassword( pathPassword, _path.get(), group, channel, login );
            }

            if( !_issetCorrectPasswordFile( pathPassword.c_str() ) ) {
                continue;
            }

            if( type == TypeUser::CONSUMER ) {
                if( !util::Validation::isConsumerName( login ) ) {
                    continue;
                }
                groups[group][channel].consumers[login] = true;
            } else if( type == TypeUser::PRODUCER ) {
                if( !util::Validation::isProducerName( login ) ) {
                    continue;
                }
                groups[group][channel].producers[login] = true;
            }
        }

    }

    void Store::_initChannelLimitMessages( const char *group, const char *channel ) {
        std::string _pathChannelLimitMessages;
        util::constants::buildPathToChannelLimitMessages( _pathChannelLimitMessages, _path.get(), group, channel );

        if( !util::FS::fileExists( _pathChannelLimitMessages.c_str() ) ) {
            util::File file( _pathChannelLimitMessages.c_str(), true );
        }

        util::File file( _pathChannelLimitMessages.c_str() );
        util::types::ChannelLimitMessages limitMessages;

        auto size = sizeof( util::types::ChannelLimitMessages );
        if( file.size() < size ) {
            file.expand( size - file.size() );
        }


        file.read( &limitMessages, sizeof( util::types::ChannelLimitMessages ) );
        limitMessages.minMessageSize = ntohl( limitMessages.minMessageSize );
        limitMessages.maxMessageSize = ntohl( limitMessages.maxMessageSize );
        limitMessages.maxMessagesInMemory = ntohl( limitMessages.maxMessagesInMemory );
        limitMessages.maxMessagesOnDisk = ntohl( limitMessages.maxMessagesOnDisk );


        if( limitMessages.minMessageSize == 0 ) {
            limitMessages.minMessageSize = 1;
        }

        if( limitMessages.minMessageSize > limitMessages.maxMessageSize ) {
            auto tmpSize = limitMessages.maxMessageSize;
            limitMessages.maxMessageSize = limitMessages.minMessageSize;
            limitMessages.minMessageSize = tmpSize;
        }

        if( limitMessages.maxMessagesInMemory == 0 && limitMessages.maxMessagesOnDisk == 0 ) {
            limitMessages.maxMessagesInMemory = 100;
        }

        unsigned long int maxMessages = limitMessages.maxMessagesOnDisk + limitMessages.maxMessagesInMemory;
        if( maxMessages > 0xFF'FF'FF'FF ) {
            limitMessages.maxMessagesInMemory = 100;
            limitMessages.maxMessagesOnDisk = 0;
        }

        groups[group][channel].limitMessages = limitMessages;

        limitMessages.minMessageSize = htonl( limitMessages.minMessageSize );
        limitMessages.maxMessageSize = htonl( limitMessages.maxMessageSize );
        limitMessages.maxMessagesInMemory = htonl( limitMessages.maxMessagesInMemory );
        limitMessages.maxMessagesOnDisk = htonl( limitMessages.maxMessagesOnDisk );

        file.write( &limitMessages, size, 0 );

    }

    void Store::_initChannels( const char *group ) {
        std::vector<std::string> dirs;
        std::string pathToGroup;
        util::constants::buildPathToGroup( pathToGroup, _path.get(), group );

        util::FS::dirs( pathToGroup.c_str(), dirs );

        for( auto it = dirs.begin(); it != dirs.end(); it++ ) {

            if( !util::Validation::isChannelName( (*it).c_str() ) ) {
                continue;
            }

            auto _channel = (*it).c_str();

            Channel channel;
            groups[group][_channel] = channel;
            _initChannelLimitMessages( group, (*it).c_str() );

            std::string _pathConsumers;
            util::constants::buildPathToConsumers( _pathConsumers, _path.get(), group, _channel );

            if( !util::FS::dirExists( _pathConsumers.c_str() ) ) {
                if( !util::FS::createDir( _pathConsumers.c_str() ) ) {
                    throw util::Error::FS_ERROR;
                }
            }

            _initUsers( group, _channel, TypeUser::CONSUMER );

            std::string _pathProducers;
            util::constants::buildPathToProducers( _pathProducers, _path.get(), group, _channel );

            if( !util::FS::dirExists( _pathProducers.c_str() ) ) {
                if( !util::FS::createDir( _pathProducers.c_str() ) ) {
                    throw util::Error::FS_ERROR;
                }
            }

            _initUsers( group, _channel, TypeUser::PRODUCER );
        }
    }

    void Store::_initGroups() {
        std::string strPathGroups;
        util::constants::buildPathToGroups( strPathGroups, _path.get() );

        const char *_pathGroups = strPathGroups.c_str();

        if( !util::FS::dirExists( _pathGroups ) ) {
            if( !util::FS::createDir( _pathGroups ) ) {
                throw util::Error::FS_ERROR;
            }
        }

        groups.clear();

        std::vector<std::string> dirs;
        util::FS::dirs( _pathGroups, dirs );

        for( auto it = dirs.begin(); it != dirs.end(); it++ ) {
            if( !util::Validation::isGroupName( (*it).c_str() ) ) {
                continue;
            }

            auto group = (*it).c_str();

            std::string _pathPassword;
            util::constants::buildPathToGroupPassword( _pathPassword, _path.get(), group );

            if( !_issetCorrectPasswordFile( _pathPassword.c_str() ) ) {
                continue;
            }

            std::map<std::string, Channel> channels;
            groups[*it] = channels;

            _initChannels( group );

        }
    }

    unsigned short int Store::getCountThreads() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );

        return ntohs( settings.countThreads );
    }

    void Store::updateCountThreads( unsigned short int countThreads ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;
        const unsigned int hc = std::thread::hardware_concurrency();

        if( countThreads == 0 ) {
            countThreads = 1;
        } else if( countThreads > hc + hc / 2 ) {
            countThreads = hc + hc / 2;
        }

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        settings.countThreads = htons( countThreads );
        file.atomicWrite( &settings, sizeof( Settings ) );
    }

    unsigned short int Store::getPort() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );

        return ntohs( settings.port );
    }

    void Store::updatePort( unsigned short int port ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        if( port == 0 || port > 65535 ) {
            port = DEFAULT_PORT;
        }

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        settings.port = htons( port );
        file.atomicWrite( &settings, sizeof( Settings ) );
    }

    void Store::getMasterPassword( unsigned char *password ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        memcpy( password, settings.password, crypto::HASH_LENGTH );
    }

    void Store::updateMasterPassword( const unsigned char *password ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        util::File file( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        memcpy( settings.password, password, crypto::HASH_LENGTH );
        file.atomicWrite( &settings, sizeof( Settings ) );

    }

    void Store::getDirectGroups( std::vector<std::string> &list ) {
        std::string path;
        util::constants::buildPathToGroups( path, _path.get() );

        std::vector<std::string> _groups;
        util::FS::dirs( path.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isGroupName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectChannels( const char *group, std::vector<std::string> &list ) {
        std::string path;
        util::constants::buildPathToGroup( path, _path.get(), group );

        std::vector<std::string> _groups;
        util::FS::dirs( path.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isChannelName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        std::string path;
        util::constants::buildPathToChannelLimitMessages( path, _path.get(), group, channel );

        {
            util::File file( path.c_str() );
            file.read( &limitMessages, sizeof( util::types::ChannelLimitMessages ) );
        }

        limitMessages.minMessageSize = ntohl( limitMessages.minMessageSize );
        limitMessages.maxMessageSize = ntohl( limitMessages.maxMessageSize );
        limitMessages.maxMessagesInMemory = ntohl( limitMessages.maxMessagesInMemory );
        limitMessages.maxMessagesOnDisk = ntohl( limitMessages.maxMessagesOnDisk );
    }

    void Store::getDirectConsumers( const char *group, const char *channel, std::vector<std::string> &list ) {
        std::string path;
        util::constants::buildPathToConsumers( path, _path.get(), group, channel );

        std::vector<std::string> _groups;
        util::FS::dirs( path.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isConsumerName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectProducers( const char *group, const char *channel, std::vector<std::string> &list ) {
        std::string path;
        util::constants::buildPathToProducers( path, _path.get(), group, channel );

        std::vector<std::string> _groups;
        util::FS::dirs( path.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isProducerName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    unsigned short int Store::getDirectPort() {
        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        Settings settings;
        {
            util::File file( path.c_str() );
            file.read( &settings, sizeof( settings ) );
        }

        return ntohs( settings.port );
    }

    unsigned short int Store::getDirectCountThreads() {
        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        Settings settings;
        {
            util::File file( path.c_str() );
            file.read( &settings, sizeof( settings ) );
        }

        return ntohs( settings.countThreads );
    }

    void Store::getDirectMasterPassword(
        unsigned char *password
    ) {
        std::string path;
        util::constants::buildPathToSettings( path, _path.get() );

        Settings settings;
        {
            util::File file( path.c_str() );
            file.read( &settings, sizeof( settings ) );
        }

        memcpy( password, settings.password, crypto::HASH_LENGTH );
    }

    void Store::getGroups( std::vector<std::string> &list ) {
        std::lock_guard<std::mutex> lock( m );

        for( auto it = groups.begin(); it != groups.end(); it++ ) {
            list.push_back( it->first );
        }
    }

    void Store::_getPassword( const char *path, unsigned char *password ) {
        util::File file( path );
        file.read( password, crypto::HASH_LENGTH );
    }

    void Store::getGroupPassword( const char *group, unsigned char *password ) {
        std::lock_guard<std::mutex> lock( m );

        auto it = groups.find( group );

        if( it == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        std::string path;
        util::constants::buildPathToGroupPassword( path, _path.get(), group );

        _getPassword( path.c_str(), password );
    }

    void Store::addGroup( const char *group, const unsigned char *password ) {
        std::lock_guard<std::mutex> lock( m );

        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_PARAM;
        }

        auto it = groups.find( group );

        if( it != groups.end() ) {
            throw util::Error::DUPLICATE_GROUP;
        }

        std::string path;
        util::constants::buildPathToGroup( path, _path.get(), group );

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        util::constants::buildPathToGroupPassword( path, _path.get(), group );

        util::File file( path.c_str(), true );
        file.write( (void *)password, crypto::HASH_LENGTH );

        std::map<std::string, Channel> channel;
        groups[group] = channel;
    }

    void Store::updateGroupPassword( const char *group, const unsigned char *password ) {
        std::lock_guard<std::mutex> lock( m );

        auto it = groups.find( group );

        if( it == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        std::string path;
        util::constants::buildPathToGroupPassword( path, _path.get(), group );

        util::File file( path.c_str(), true );
        file.atomicWrite( (void *)password, crypto::HASH_LENGTH );
    }

    void Store::removeGroup( const char *group ) {
        std::lock_guard<std::mutex> lock( m );

        auto it = groups.find( group );

        if( it == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        std::string path;
        util::constants::buildPathToGroup( path, _path.get(), group );

        util::FS::removeDir( path.c_str() );

        groups.erase( it );
    }

    void Store::getChannels( const char *group, std::list<std::string> &channels ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        for( auto it = groups[group].begin(); it != groups[group].end(); it++ ) {
            channels.push_back( it->first );
        }
    }

    void Store::addChannel(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel != itGroup->second.end() ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }

        std::string path;
        util::constants::buildPathToChannel( path, _path.get(), group, channel );

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        std::string pathLimitMessages;
        util::constants::buildPathToChannelLimitMessages( pathLimitMessages, _path.get(), group, channel );

        util::File fileSettings( pathLimitMessages.c_str(), true );
        util::types::ChannelLimitMessages channelLimitMessages;

        channelLimitMessages.minMessageSize = htonl( limitMessages.minMessageSize );
        channelLimitMessages.maxMessageSize = htonl( limitMessages.maxMessageSize );
        channelLimitMessages.maxMessagesInMemory = htonl( limitMessages.maxMessagesInMemory );
        channelLimitMessages.maxMessagesOnDisk = htonl( limitMessages.maxMessagesOnDisk );

        fileSettings.write( &channelLimitMessages, sizeof( util::types::ChannelLimitMessages ) );


        std::string _pathConsumers;
        util::constants::buildPathToConsumers( _pathConsumers, _path.get(), group, channel );

        if( !util::FS::createDir( _pathConsumers.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        std::string _pathProducers;
        util::constants::buildPathToProducers( _pathProducers, _path.get(), group, channel );

        if( !util::FS::createDir( _pathProducers.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        Channel ch;
        ch.limitMessages = limitMessages;
        groups[group][channel] = ch;

    }

    void Store::removeChannel( const char *group, const char *channel ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            return;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            return;
        }

        std::string path;
        util::constants::buildPathToChannel( path, _path.get(), group, channel );

        util::FS::removeDir( path.c_str() );

        groups[group].erase( itChannel );
    }

    void Store::getChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages &limitMessages
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        limitMessages = groups[group][channel].limitMessages;
    }

    void Store::updateChannelLimitMessages(
        const char *group,
        const char *channel,
        util::types::ChannelLimitMessages limitMessages
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        std::string path;
        util::constants::buildPathToChannelLimitMessages( path, _path.get(), group, channel );

        groups[group][channel].limitMessages = limitMessages;

        util::File fileSettings( path.c_str() );

        limitMessages.minMessageSize = htonl( limitMessages.minMessageSize );
        limitMessages.maxMessageSize = htonl( limitMessages.maxMessageSize );
        limitMessages.maxMessagesInMemory = htonl( limitMessages.maxMessagesInMemory );
        limitMessages.maxMessagesOnDisk = htonl( limitMessages.maxMessagesOnDisk );

        fileSettings.atomicWrite( &limitMessages, sizeof( util::types::ChannelLimitMessages ) );
    }

    void Store::getConsumers( const char *group, const char *channel, std::list<std::string> &consumers ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto _group = groups[group];

        auto itChannel = _group.find( channel );
        if( itChannel == _group.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto _channel = _group[channel];

        for( auto it = _channel.consumers.begin(); it != _channel.consumers.end(); it++ ) {
            consumers.push_back( it->first );
        }
    }

    void Store::addConsumer(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].consumers.find( login );
        if( itConsumer != groups[group][channel].consumers.end() ) {
            throw util::Error::DUPLICATE_CONSUMER;
        }

        std::string path;
        util::constants::buildPathToConsumer( path, _path.get(), group, channel, login );

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        util::constants::buildPathToConsumerPassword( path, _path.get(), group, channel, login );

        util::File file( path.c_str(), true );
        file.write( (void *)password, crypto::HASH_LENGTH );

        groups[group][channel].consumers[login] = true;
    }

    void Store::removeConsumer( const char *group, const char *channel, const char *login ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            return;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            return;
        }

        auto itConsumer = groups[group][channel].consumers.find( login );
        if( itConsumer == groups[group][channel].consumers.end() ) {
            return;
        }

        std::string path;
        util::constants::buildPathToConsumer( path, _path.get(), group, channel, login );

        util::FS::removeDir( path.c_str() );

        groups[group][channel].consumers.erase( itConsumer );
    }

    void Store::getConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].consumers.find( login );
        if( itConsumer == groups[group][channel].consumers.end() ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        std::string path;
        util::constants::buildPathToConsumerPassword( path, _path.get(), group, channel, login );

        util::File filePassword( path.c_str() );
        filePassword.read( password, crypto::HASH_LENGTH );
    }

    void Store::updateConsumerPassword(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].consumers.find( login );
        if( itConsumer == groups[group][channel].consumers.end() ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        std::string path;
        util::constants::buildPathToConsumerPassword( path, _path.get(), group, channel, login );

        util::File filePassword( path.c_str() );
        filePassword.atomicWrite( (void *)password, crypto::HASH_LENGTH );
    }

    void Store::getProducers( const char *group, const char *channel, std::list<std::string> &producers ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto _group = groups[group];

        auto itChannel = _group.find( channel );
        if( itChannel == _group.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto _channel = _group[channel];

        for( auto it = _channel.producers.begin(); it != _channel.producers.end(); it++ ) {
            producers.push_back( it->first );
        }
    }

    void Store::addProducer(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].producers.find( login );
        if( itConsumer != groups[group][channel].producers.end() ) {
            throw util::Error::DUPLICATE_PRODUCER;
        }

        std::string path;
        util::constants::buildPathToProducer( path, _path.get(), group, channel, login );

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        util::constants::buildPathToProducerPassword( path, _path.get(), group, channel, login );

        util::File file( path.c_str(), true );
        file.write( (void *)password, crypto::HASH_LENGTH );

        groups[group][channel].producers[login] = true;
    }

    void Store::removeProducer( const char *group, const char *channel, const char *login ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            return;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            return;
        }

        auto itConsumer = groups[group][channel].producers.find( login );
        if( itConsumer == groups[group][channel].producers.end() ) {
            return;
        }

        std::string path;
        util::constants::buildPathToProducer( path, _path.get(), group, channel, login );

        util::FS::removeDir( path.c_str() );

        groups[group][channel].producers.erase( itConsumer );
    }

    void Store::getProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].producers.find( login );
        if( itConsumer == groups[group][channel].producers.end() ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }

        std::string path;
        util::constants::buildPathToProducerPassword( path, _path.get(), group, channel, login );

        util::File filePassword( path.c_str() );
        filePassword.read( password, crypto::HASH_LENGTH );
    }

    void Store::updateProducerPassword(
        const char *group,
        const char *channel,
        const char *login,
        const unsigned char *password
    ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        auto itConsumer = groups[group][channel].producers.find( login );
        if( itConsumer == groups[group][channel].producers.end() ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }

        std::string path;
        util::constants::buildPathToProducerPassword( path, _path.get(), group, channel, login );

        util::File filePassword( path.c_str() );
        filePassword.atomicWrite( (void *)password, crypto::HASH_LENGTH );
    }
}

#endif
