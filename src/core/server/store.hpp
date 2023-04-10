#ifndef SIMQ_CORE_SERVER_STORE 
#define SIMQ_CORE_SERVER_STORE 

#include <string>
#include <string.h>
#include <mutex>
#include <vector>
#include <thread>
#include <arpa/inet.h>
#include <map>
#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/validation.hpp"
#include "../../util/fs.hpp"
#include "../../util/file.hpp"
#include "../../util/error.h"


namespace simq::core::server {
    class Store {
        private:
            const char *pathConsumers = "consumers";
            const char *pathProducers = "producers";
            const char *pathSettings = "settings";
            const char *pathChannels = "channels";
            const char *pathGroups = "groups";
            const char *fileSettings = "settings";
            const char *filePassword = "password";

            const unsigned int DEFAULT_PORT = 4012;

            struct Settings {
                unsigned short int countThreads;
                unsigned short int port;
                unsigned char password[crypto::HASH_LENGTH];
            };

            std::mutex m;

            struct Channel {
                std::map<std::string, bool> consumers;
                std::map<std::string, bool> producers;
            };

            std::map<std::string, std::map<std::string, Channel>> groups;

            enum TypeUser {
                CONSUMER,
                PRODUCER,
            };

            char *_path = nullptr;

            void _initSettings( const char *path );
            void _initChannels( const char *group, const char *path );
            void _initChannelSettings( const char *path, const char *channel );
            void _initUsers( const char *group, const char *channel, TypeUser type, const char *path );
            void _initGroups( const char *path );
            bool _issetCorrectPasswordFile( const char *path );
            void _getPassword( const char *path, unsigned char password[crypto::HASH_LENGTH] );

            void _getFileSettings( std::string &path );
        public:
            Store( const char *path );

            void getDirectGroups( std::vector<std::string> &list );
            void getDirectChannels( const char *group, std::vector<std::string> &list );
            void getDirectChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void getDirectConsumers( const char *group, const char *channel, std::vector<std::string> &list );
            void getDirectProducers( const char *group, const char *channel, std::vector<std::string> &list );
            unsigned short int getDirectPort();
            unsigned short int getDirectCountThreads();
            void getDirectMasterPassword(
                unsigned char password[crypto::HASH_LENGTH]
            );

            void getGroups( std::vector<std::string> &groups );
            void getGroupPassword( const char *group, unsigned char password[crypto::HASH_LENGTH] );
            void addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] );
            void removeGroup( const char *group );

            void getChannels( const char *group, std::vector<std::string> &channels );
            void getChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void addChannel( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void updateChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void removeChannel( const char *group, const char *channel );

            void getConsumers( const char *group, const char *channel, std::vector<std::string> &consumers );
            void addConsumer( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void getConsumerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void updateConsumerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void removeConsumer( const char *group, const char *channel, const char *login );

            void getProducers( const char *group, const char *channel, std::vector<std::string> &producers );
            void addProducer( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void getProducerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void updateProducerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] );
            void removeProducer( const char *group, const char *channel, const char *login );

            void getMasterPassword( unsigned char password[crypto::HASH_LENGTH] );
            void updateMasterPassword( const unsigned char password[crypto::HASH_LENGTH] );

            unsigned short int getCountThreads();
            void updateCountThreads( unsigned short int count );

            unsigned short int getPort();
            void updatePort( unsigned short int port );
    };

    Store::Store( const char *path ) {
        auto length = strlen( path );
        _path = new char[length + 1]{};
        memcpy( _path, path, length );

        _initSettings( path );
        _initGroups( path );
    }

    void Store::_initSettings( const char *path ) {
        Settings settings;

        bool isNewSettings = false;
        const unsigned int hc = std::thread::hardware_concurrency();

        std::string pathToDir = path;
        pathToDir += "/";
        pathToDir += pathSettings;
        const char *_pathToDir = pathToDir.c_str();

        std::string pathToFile;
        _getFileSettings( pathToFile );
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
        if( file.size() < crypto::HASH_LENGTH ) {
            return false;
        }
        
        return true;
    }

    void Store::_initUsers( const char *group, const char *channel, TypeUser type, const char *path ) {
        std::vector<std::string> dirs;

        std::string _path = path;

        util::FS::dirs( _path.c_str(), dirs );

        for( auto it = dirs.begin(); it != dirs.end(); it++ ) {
            std::string _path = path;
            _path += "/";
            _path += *it;
            _path += "/";
            _path += filePassword;

            if( !_issetCorrectPasswordFile( _path.c_str() ) ) {
                continue;
            }


            if( type == TypeUser::CONSUMER ) {
                if( !util::Validation::isConsumerName( (*it).c_str() ) ) {
                    continue;
                }
                groups[group][channel].consumers[*it] = true;
            } else if( type == TypeUser::PRODUCER ) {
                if( !util::Validation::isProducerName( (*it).c_str() ) ) {
                    continue;
                }
                groups[group][channel].producers[*it] = true;
            }
        }

    }

    void Store::_initChannelSettings( const char *path, const char *channel ) {
        std::string _pathChannelSettings = path;
        _pathChannelSettings += "/";
        _pathChannelSettings += channel;
        _pathChannelSettings += "/";
        _pathChannelSettings += pathSettings;

        if( !util::FS::fileExists( _pathChannelSettings.c_str() ) ) {
            util::File file( _pathChannelSettings.c_str(), true );
        }

        util::File file( _pathChannelSettings.c_str() );
        util::Types::ChannelSettings settings;

        auto size = sizeof( util::Types::ChannelSettings );
        if( file.size() < size ) {
            file.expand( size - file.size() );
        }


        file.read( &settings, sizeof( util::Types::ChannelSettings ) );
        settings.minMessageSize = ntohl( settings.minMessageSize );
        settings.maxMessageSize = ntohl( settings.maxMessageSize );
        settings.maxMessagesInMemory = ntohl( settings.maxMessagesInMemory );
        settings.maxMessagesOnDisk = ntohl( settings.maxMessagesOnDisk );


        if( settings.minMessageSize == 0 ) {
            settings.minMessageSize = 1;
        }

        if( settings.minMessageSize > settings.maxMessageSize ) {
            auto tmpSize = settings.maxMessageSize;
            settings.maxMessageSize = settings.minMessageSize;
            settings.minMessageSize = tmpSize;
        }

        if( settings.maxMessagesInMemory == 0 && settings.maxMessagesOnDisk == 0 ) {
            settings.maxMessagesInMemory = 100;
        }

        unsigned long int maxMessages = settings.maxMessagesOnDisk + settings.maxMessagesInMemory;
        if( maxMessages > 0xFF'FF'FF'FF ) {
            settings.maxMessagesInMemory = 100;
            settings.maxMessagesOnDisk = 0;
        }

        settings.minMessageSize = htonl( settings.minMessageSize );
        settings.maxMessageSize = htonl( settings.maxMessageSize );
        settings.maxMessagesInMemory = htonl( settings.maxMessagesInMemory );
        settings.maxMessagesOnDisk = htonl( settings.maxMessagesOnDisk );

        file.write( &settings, size, 0 );
    }

    void Store::_initChannels( const char *group, const char *path ) {
        std::vector<std::string> dirs;
        std::string _path = path;

        util::FS::dirs( _path.c_str(), dirs );

        for( auto it = dirs.begin(); it != dirs.end(); it++ ) {

            if( !util::Validation::isChannelName( (*it).c_str() ) ) {
                continue;
            }

            _initChannelSettings( path, (*it).c_str() );

            Channel channel;
            groups[group][*it] = channel;


            auto _pathConsumers = _path;
            _pathConsumers += "/";
            _pathConsumers += *it;
            _pathConsumers += "/";
            _pathConsumers += pathConsumers;


            if( !util::FS::dirExists( _pathConsumers.c_str() ) ) {
                if( !util::FS::createDir( _pathConsumers.c_str() ) ) {
                    throw util::Error::FS_ERROR;
                }
            }

            _initUsers( group, (*it).c_str(), TypeUser::CONSUMER, _pathConsumers.c_str() );


            auto _pathProducers = _path;
            _pathProducers += "/";
            _pathProducers += *it;
            _pathProducers += "/";
            _pathProducers += pathProducers;

            if( !util::FS::dirExists( _pathProducers.c_str() ) ) {
                if( !util::FS::createDir( _pathProducers.c_str() ) ) {
                    throw util::Error::FS_ERROR;
                }
            }

            _initUsers( group, (*it).c_str(), TypeUser::PRODUCER, _pathProducers.c_str() );
        }
    }

    void Store::_initGroups( const char *path ) {
        std::string strPathGroups = path;
        strPathGroups += "/";
        strPathGroups += pathGroups;
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

            std::string _pathPassword = _pathGroups;
            _pathPassword += "/";
            _pathPassword += *it;
            _pathPassword += "/";
            _pathPassword += filePassword;

            if( !_issetCorrectPasswordFile( _pathPassword.c_str() ) ) {
                continue;
            }

            std::string _path = _pathGroups;
            _path += "/";
            _path += *it;
            _initChannels( (*it).c_str(), _path.c_str() );

            std::map<std::string, Channel> channels;
            groups[*it] = channels;
        }
    }

    void Store::_getFileSettings( std::string &path ) {
        path = _path;
        path += "/";
        path += pathSettings;
        path += "/";
        path += fileSettings;
    }

    unsigned short int Store::getCountThreads() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
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
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        settings.countThreads = htons( countThreads );
        file.write( &settings, sizeof( Settings ), 0 );
    }

    unsigned short int Store::getPort() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
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
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        settings.port = htons( port );
        file.write( &settings, sizeof( Settings ), 0 );
    }

    void Store::getMasterPassword( unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        memcpy( password, settings.password, crypto::HASH_LENGTH );
    }

    void Store::updateMasterPassword( const unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::File( path.c_str() );
        file.read( &settings, sizeof( Settings ) );
        memcpy( settings.password, password, crypto::HASH_LENGTH );
        file.write( &settings, sizeof( Settings ), 0 );

    }

    void Store::getDirectGroups( std::vector<std::string> &list ) {
        std::string p = _path;
        p += "/";
        p += pathGroups;

        std::vector<std::string> _groups;
        util::FS::dirs( p.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isGroupName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectChannels( const char *group, std::vector<std::string> &list ) {
        std::string p = _path;
        p += "/";
        p += pathGroups;
        p += "/";
        p += group;

        std::vector<std::string> _groups;
        util::FS::dirs( p.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isChannelName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings ) {
        std::string p = _path;
        p += "/";
        p += pathGroups;
        p += "/";
        p += group;
        p += "/";
        p += channel;
        p += "/";
        p += fileSettings;

        {
            auto file = util::File( p.c_str() );
            file.read( &settings, sizeof( util::Types::ChannelSettings ) );
        }

        settings.minMessageSize = ntohl( settings.minMessageSize );
        settings.maxMessageSize = ntohl( settings.maxMessageSize );
        settings.maxMessagesInMemory = ntohl( settings.maxMessagesInMemory );
        settings.maxMessagesOnDisk = ntohl( settings.maxMessagesOnDisk );
    }

    void Store::getDirectConsumers( const char *group, const char *channel, std::vector<std::string> &list ) {
        std::string p = _path;
        p += "/";
        p += pathGroups;
        p += "/";
        p += group;
        p += "/";
        p += channel;
        p += "/";
        p += pathConsumers;

        std::vector<std::string> _groups;
        util::FS::dirs( p.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isConsumerName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    void Store::getDirectProducers( const char *group, const char *channel, std::vector<std::string> &list ) {
        std::string p = _path;
        p += "/";
        p += pathGroups;
        p += "/";
        p += group;
        p += "/";
        p += channel;
        p += "/";
        p += pathProducers;

        std::vector<std::string> _groups;
        util::FS::dirs( p.c_str(), _groups );

        for( auto it = _groups.begin(); it != _groups.end(); it++ ) {
            if( util::Validation::isProducerName( (*it).c_str() ) ) {
                list.push_back( *it );
            }
        }
    }

    unsigned short int Store::getDirectPort() {
        std::string p = _path;
        p += "/";
        p += pathSettings;
        p += "/";
        p += fileSettings;

        Settings settings;
        {
            auto file = util::File( p.c_str() );
            file.read( &settings, sizeof( settings ) );
        }

        return ntohs( settings.port );
    }

    unsigned short int Store::getDirectCountThreads() {
        std::string p = _path;
        p += "/";
        p += pathSettings;
        p += "/";
        p += fileSettings;

        Settings settings;
        {
            auto file = util::File( p.c_str() );
            file.read( &settings, sizeof( settings ) );
        }

        return ntohs( settings.countThreads );
    }

    void Store::getDirectMasterPassword(
        unsigned char password[crypto::HASH_LENGTH]
    ) {
        std::string p = _path;
        p += "/";
        p += pathSettings;
        p += "/";
        p += fileSettings;

        Settings settings;
        {
            auto file = util::File( p.c_str() );
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

    void Store::_getPassword( const char *path, unsigned char password[crypto::HASH_LENGTH] ) {
        util::File file( path );
        file.read( password, crypto::HASH_LENGTH );
    }

    void Store::getGroupPassword( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );

        auto it = groups.find( group );

        if( it == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        std::string path = _path;
        path += "/";
        path += group;
        path += "/";
        path += filePassword;

        _getPassword( path.c_str(), password );
    }

    void Store::addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );

        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_PARAM;
        }

        auto it = groups.find( group );

        if( it != groups.end() ) {
            throw util::Error::DUPLICATE_GROUP;
        }

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        path += "/";
        path += filePassword;

        util::File file( path.c_str(), true );
        file.write( password, crypto::HASH_LENGTH );

        std::map<std::string, Channel> channel;
        groups[group] = channel;
    }

    void Store::removeGroup( const char *group ) {
        std::lock_guard<std::mutex> lock( m );

        auto it = groups.find( group );

        if( it == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;

        util::FS::removeDir( path.c_str() );

        groups.erase( it );
    }

    void Store::getChannels( const char *group, std::vector<std::string> &channels ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        for( auto it = groups[group].begin(); it != groups[group].end(); it++ ) {
            channels.push_back( it->first );

        }
    }

    void Store::addChannel( const char *group, const char *channel, util::Types::ChannelSettings &settings ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel != itGroup->second.end() ) {
            throw util::Error::DUPLICATE_CHANNEL;
        }

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        auto pathSettings = path;
        pathSettings += "/";
        pathSettings += fileSettings;

        util::File fileSettings( pathSettings.c_str(), true );
        util::Types::ChannelSettings channelSettings;

        channelSettings.minMessageSize = htonl( settings.minMessageSize );
        channelSettings.maxMessageSize = htonl( settings.maxMessageSize );
        channelSettings.maxMessagesInMemory = htonl( settings.maxMessagesInMemory );
        channelSettings.maxMessagesOnDisk = htonl( settings.maxMessagesOnDisk );

        fileSettings.write( &channelSettings, sizeof( util::Types::ChannelSettings ) );


        auto _pathConsumers = path;
        _pathConsumers += "/";
        _pathConsumers += pathConsumers;
        if( !util::FS::createDir( _pathConsumers.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        auto _pathProducers = path;
        _pathProducers += "/";
        _pathProducers += pathProducers;
        if( !util::FS::createDir( _pathProducers.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        Channel ch;
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;

        util::FS::removeDir( path.c_str() );

        groups[group].erase( itChannel );
    }

    void Store::getChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += fileSettings;

        util::File fileSettings( path.c_str() );
        fileSettings.read( &settings, sizeof( util::Types::ChannelSettings ) );

        settings.minMessageSize = ntohl( settings.minMessageSize );
        settings.maxMessageSize = ntohl( settings.maxMessageSize );
        settings.maxMessagesInMemory = ntohl( settings.maxMessagesInMemory );
        settings.maxMessagesOnDisk = ntohl( settings.maxMessagesOnDisk );
    }

    void Store::updateChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += fileSettings;

        util::File fileSettings( path.c_str() );

        settings.minMessageSize = htonl( settings.minMessageSize );
        settings.maxMessageSize = htonl( settings.maxMessageSize );
        settings.maxMessagesInMemory = htonl( settings.maxMessagesInMemory );
        settings.maxMessagesOnDisk = htonl( settings.maxMessagesOnDisk );

        fileSettings.write( &settings, sizeof( util::Types::ChannelSettings ) );
    }

    void Store::getConsumers( const char *group, const char *channel, std::vector<std::string> &consumers ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        for( auto it = itChannel->second.consumers.begin(); it != itChannel->second.consumers.end(); it++ ) {
            consumers.push_back( it->first );
        }
    }

    void Store::addConsumer( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathConsumers;
        path += "/";
        path += login;

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        path += "/";
        path += filePassword;

        util::File file( path.c_str(), true );
        file.write( password, crypto::HASH_LENGTH );

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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathConsumers;
        path += "/";
        path += login;

        util::FS::removeDir( path.c_str() );

        groups[group][channel].consumers.erase( itConsumer );
    }

    void Store::getConsumerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathConsumers;
        path += "/";
        path += login;
        path += "/";
        path += filePassword;

        util::File filePassword( path.c_str() );
        filePassword.read( password, crypto::HASH_LENGTH );
    }

    void Store::updateConsumerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathConsumers;
        path += "/";
        path += login;
        path += "/";
        path += filePassword;

        util::File filePassword( path.c_str() );
        filePassword.write( password, crypto::HASH_LENGTH );
    }

    void Store::getProducers( const char *group, const char *channel, std::vector<std::string> &producers ) {
        std::lock_guard<std::mutex> lock( m );

        auto itGroup = groups.find( group );

        if( itGroup == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        }

        auto itChannel = itGroup->second.find( channel );
        if( itChannel == itGroup->second.end() ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        }

        for( auto it = itChannel->second.producers.begin(); it != itChannel->second.consumers.end(); it++ ) {
            producers.push_back( it->first );
        }
    }

    void Store::addProducer( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathProducers;
        path += "/";
        path += login;

        if( !util::FS::createDir( path.c_str() ) ) {
            throw util::Error::FS_ERROR;
        }

        path += "/";
        path += filePassword;

        util::File file( path.c_str(), true );
        file.write( password, crypto::HASH_LENGTH );

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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathProducers;
        path += "/";
        path += login;

        util::FS::removeDir( path.c_str() );

        groups[group][channel].producers.erase( itConsumer );
    }

    void Store::getProducerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathProducers;
        path += "/";
        path += login;
        path += "/";
        path += filePassword;

        util::File filePassword( path.c_str() );
        filePassword.read( password, crypto::HASH_LENGTH );
    }

    void Store::updateProducerPassword( const char *group, const char *channel, const char *login, unsigned char password[crypto::HASH_LENGTH] ) {
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

        std::string path = _path;
        path += "/";
        path += pathGroups;
        path += "/";
        path += group;
        path += "/";
        path += channel;
        path += "/";
        path += pathProducers;
        path += "/";
        path += login;
        path += "/";
        path += filePassword;

        util::File filePassword( path.c_str() );
        filePassword.write( password, crypto::HASH_LENGTH );
    }
}

#endif
