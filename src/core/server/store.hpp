#ifndef SIMQ_CORE_SERVER_STORE 
#define SIMQ_CORE_SERVER_STORE 

#include <list>
#include <string>
#include <string.h>
#include <mutex>
#include <thread>
#include <arpa/inet.h>
#include <iostream>
#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/fs.hpp"
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
                unsigned int countThreads;
                unsigned int port;
                unsigned char password[crypto::HASH_LENGTH];
            };

            std::mutex m;

            char *_path = nullptr;

            void _initSettings( const char *path );
            void _initChannels( const char *path );
            void _initUsers( const char *path );
            void _initGroups( const char *path );

            void _getFileSettings( std::string &path );
        public:
            Store( const char *path );

            void getGroups( std::list<std::string> &groups );
            void getGroupPassword( const char *group, char password[crypto::HASH_LENGTH] );
            void addGroup( const char *group, char password[crypto::HASH_LENGTH] );
            void removeGroup( const char *group );

            void getChannels( const char *group, std::list<std::string> &channels );
            void getChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void addChannel( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void updateChannelSettings( const char *group, const char *channel, util::Types::ChannelSettings &settings );
            void removeChannel( const char *group, const char *channel );

            void getConsumers( const char *group, const char *channel, std::list<std::string> &consumers );
            void getConsumerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void updateConsumerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void removeConsumer( const char *group, const char *channel, const char *login );

            void getProduces( const char *group, const char *channel, std::list<std::string> &producers );
            void getProducerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void updateProducerPassword( const char *group, const char *channel, const char *login, char password[crypto::HASH_LENGTH] );
            void removeProducer( const char *group, const char *channel, const char *login );

            void getMasterPassword( unsigned char password[crypto::HASH_LENGTH] );
            void updateMasterPassword( unsigned char password[crypto::HASH_LENGTH] );

            unsigned int getCountThreads();
            void updateCountThreads( unsigned int count );

            unsigned int getPort();
            void updatePort( unsigned int port );
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
            if( !util::FS::createFile( _pathToFile ) ) {
                throw util::Error::FS_ERROR;
            }
            isNewSettings = true;
        } else if( !util::FS::fileExists( _pathToFile ) ) {
            if( !util::FS::createFile( _pathToFile ) ) {
                throw util::Error::FS_ERROR;
            }
            isNewSettings = true;
        } else if( util::FS::getFileSize( _pathToFile ) < sizeof( Settings ) ) {
            isNewSettings = true;
        }


        auto file = util::FS::openFile( _pathToFile );

        if( file == NULL ) {
            throw util::Error::FS_ERROR;
        }

        if( isNewSettings ) {
            settings.countThreads = htonl( hc );
            settings.port = htonl( DEFAULT_PORT );
            crypto::Hash::hash( "simq", settings.password );

            util::FS::writeFile( file, 0, sizeof( Settings ), &settings );
            util::FS::closeFile( file );
        } else {
            util::FS::readFile( file, 0, sizeof( Settings ), &settings );

            settings.countThreads = ntohl( settings.countThreads );
            settings.port = ntohl( settings.port );

            bool isWrong = false;

            if( settings.countThreads > hc + hc / 2 ) {
                settings.countThreads = htonl( hc + hc / 2 );
                isWrong = true;
            } else if( settings.countThreads == 0 ) {
                settings.countThreads = htonl( 1 );
                isWrong = true;
            }

            if( settings.port > 65535 || settings.port == 0 ) {
                isWrong = true;
                settings.port = htonl( DEFAULT_PORT );
            }

            if( isWrong ) {
                util::FS::writeFile( file, 0, sizeof( Settings ), &settings );
            }

            util::FS::closeFile( file );
        }


    }

    void Store::_initGroups( const char *path ) {
    }

    void Store::_getFileSettings( std::string &path ) {
        path = _path;
        path += "/";
        path += pathSettings;
        path += "/";
        path += fileSettings;
    }

    unsigned int Store::getCountThreads() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::FS::openFile( path.c_str() );
        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        util::FS::closeFile( file );

        return ntohl( settings.countThreads );
    }

    void Store::updateCountThreads( unsigned int countThreads ) {
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

        auto file = util::FS::openFile( path.c_str() );

        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        settings.countThreads = htonl( countThreads );
        util::FS::writeFile( file, 0, sizeof( Settings ), &settings );
        util::FS::closeFile( file );
    }

    unsigned int Store::getPort() {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::FS::openFile( path.c_str() );
        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        util::FS::closeFile( file );

        return ntohl( settings.port );
    }

    void Store::updatePort( unsigned int port ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        if( port == 0 || port > 65535 ) {
            port = DEFAULT_PORT;
        }

        std::string path;
        _getFileSettings( path );

        auto file = util::FS::openFile( path.c_str() );
        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        settings.port = htonl( port );
        util::FS::writeFile( file, 0, sizeof( Settings ), &settings );
        util::FS::closeFile( file );
    }

    void Store::getMasterPassword( unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::FS::openFile( path.c_str() );
        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        memcpy( password, settings.password, crypto::HASH_LENGTH );
        util::FS::closeFile( file );
    }

    void Store::updateMasterPassword( unsigned char password[crypto::HASH_LENGTH] ) {
        std::lock_guard<std::mutex> lock( m );
        Settings settings;

        std::string path;
        _getFileSettings( path );

        auto file = util::FS::openFile( path.c_str() );
        util::FS::readFile( file, 0, sizeof( Settings ), &settings );
        memcpy( settings.password, password, crypto::HASH_LENGTH );
        util::FS::writeFile( file, 0, sizeof( Settings ), &settings );
        util::FS::closeFile( file );
    }
}

#endif
