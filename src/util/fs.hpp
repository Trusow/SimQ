#ifndef SIMQ_UTIL_FS
#define SIMQ_UTIL_FS

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "validation.hpp"

namespace simq::util {
    class FS {
        public:
        enum ValidationName {
            NONE,
            GROUP,
            CHANNEL,
            CONSUMER,
            PRODUCER, 
        };
        private:
        enum Type {
            _FILE,
            _DIR,
        };
        static void sort(char** arr,const int len );
        static char **list( const char *path, unsigned int &length, ValidationName v, Type t );
        public:
        static bool dirExists( const char *path );
        static bool dirExists( unsigned int length, ... );
        static bool fileExists( const char *path );
        static bool fileExists( unsigned int length, ... );
        static bool createDir( const char *path );
        static bool createDir( unsigned int length, ... );
        static bool removeDir( const char *path );
        static bool removeDir( unsigned int length, ... );
        static bool removeFile( const char *path );
        static bool removeFile( unsigned int length, ... );
        static bool listDirs( const char *path );
        static bool listDirs( unsigned int length, ... );
        static char *buildPath( unsigned int length, ... );
        static char **dirs( const char *path, unsigned int &length, ValidationName v = NONE );
        static char **files( const char *path, unsigned int &length, ValidationName v = NONE );
        static void free( char **data, unsigned int length );
        static void free( char *path );
    };

    bool FS::dirExists( const char *path ) {
        auto *dir = opendir( path );

        if( dir == nullptr ) {
            return false;
        }

        closedir( dir );

        return true;
    }

    bool FS::fileExists( const char *path ) {
        auto *file = fopen( path, "r" );

        if( file == nullptr ) {
            return false;
        }

        fclose( file );

        return true;
    }

    bool FS::createDir( const char *path ) {
        return mkdir( path, 0700 ) == 0;
    }

    bool FS::removeDir( const char *path ) {
        return rmdir( path ) == 0;
    }

    bool FS::removeFile( const char *path ) {
        return unlink( path ) == 0;
    }

    bool FS::createDir( unsigned int length, ... ) {
        char *path = buildPath( length );
        bool isCreate = createDir( path );
        ::free( path );

        return isCreate;
    }

    bool FS::removeDir( unsigned int length, ... ) {
        char *path = buildPath( length );
        bool isRemove = removeDir( path );
        ::free( path );

        return isRemove;
    }

    bool FS::removeFile( unsigned int length, ... ) {
        char *path = buildPath( length );
        bool isRemove = removeFile( path );
        ::free( path );


        return isRemove;
    }

    bool FS::dirExists( unsigned int length, ... ) {
        char *path = buildPath( length );
        bool isExists = dirExists( path );
        ::free( path );


        return isExists;
    }

    bool FS::fileExists( unsigned int length, ... ) {
        char *path = buildPath( length );
        bool isExists = fileExists( path );
        ::free( path );


        return isExists;
    }

    char *FS::buildPath( unsigned int length, ... ) {
        va_list list;
        va_start( list, length );
        char *path = nullptr;
        unsigned int offset = 0;

        for( unsigned int i = 0; i < length; i++ ) {
            auto *part = va_arg( list, const char *);
            unsigned int l = strlen( part );
            bool isEnd = i == length - 1;

            if( path == nullptr ) {
                path = (char *)malloc( sizeof( char ) * ( l + 1 ) );
            } else {
                path = (char *)realloc( path, sizeof( char ) * ( offset + l + 1 ) );
            }

            memcpy( &path[offset], part, l );

            offset += l + 1;
            if( !isEnd ) {
                path[offset-1] = '/';
            } else {
                path[offset-1] = 0;
            }
        }

        va_end( list );

        return path;
    }

    void FS::sort( char** data,const int length ){
        int mid = length / 2;
        int con = 0;
        while(!con){
            con = 1;
            for(int i = 0; i < length - 1; i++){
                if(strcmp(data[i], data[i + 1]) > 0) {
                    char *temp = data[i];
                    data[i] = data[i + 1];
                    data[i + 1] = temp;
                    con = 0;
                }
            }
        }
    }

    char **FS::list( const char *path, unsigned int &length, ValidationName v, Type t ) {
        auto *dir = opendir( path );
        if( dir == nullptr ) {
            return nullptr;
        }
     
        char **data = nullptr;
     
        dirent *pDirent;
        while (( pDirent = readdir( dir ) ) != nullptr) {
            bool isFile = t == Type::_FILE && pDirent->d_type == DT_REG;
            bool isDir = t == Type::_DIR && pDirent->d_type == DT_DIR;
            if(  isFile || isDir ) {
                bool isValid = false;

                switch( v ) {
                    case ValidationName::GROUP:
                    isValid = Validation::isGroupName( pDirent->d_name );
                    break;
                    case ValidationName::CHANNEL:
                    isValid = Validation::isChannelName( pDirent->d_name );
                    break;
                    case ValidationName::CONSUMER:
                    isValid = Validation::isConsumerName( pDirent->d_name );
                    break;
                    case ValidationName::PRODUCER:
                    isValid = Validation::isProducerName( pDirent->d_name );
                    break;
                    default:
                    isValid = true;
                    break;
                }

                if( !isValid ) {
                    continue;
                }

                if( data == nullptr ) {
                    data = (char **)malloc( sizeof( char * ) );
                } else {
                    data = (char **)realloc( data, sizeof( char * ) * ( length + 1 ) );
                }
     
                unsigned int l = strlen( pDirent->d_name );
                data[length] = new char[l + 1];
                memcpy( data[length], pDirent->d_name, l );
                data[length][l] = 0;
                length++;
            }
        }
     
        sort( data, length );
     
        return data;
    }

    char **FS::dirs( const char *path, unsigned int &length, ValidationName v ) {
        return list( path, length, v, Type::_DIR );
    }

    char **FS::files( const char *path, unsigned int &length, ValidationName v ) {
        return list( path, length, v, Type::_FILE );
    }

    void FS::free( char **data, unsigned int length ) {
        for( unsigned int i = 0; i < length; i++ ) {
            delete[] data[i];
        }
        ::free( data );
    }

    void FS::free( char *data ) {
        ::free( data );
    }
}

#endif
