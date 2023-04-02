#ifndef SIMQ_UTIL_FILE
#define SIMQ_UTIL_FILE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "error.h"

namespace simq::util {
    class File {
        private:
            char *filePath = nullptr;
            FILE *file = NULL;
            bool exists( const char *path );
            FILE *create( const char *path );
            FILE *open( const char *path );
            void move( unsigned long int offset );

        public:
            File( const char *path, bool createIfNotExists = false );
            void write( void *data, unsigned int length );
            void write( void *data, unsigned int length, unsigned long int offset );
            unsigned int read( void *data, unsigned int length );
            unsigned int read( void *data, unsigned int length, unsigned long int offset );
            unsigned long int size();
            void expand( unsigned int size );
            ~File();
    };

    File::File( const char *path, bool createIfNotExists ) {
        if( !exists( path ) ) {
            if( createIfNotExists ) {
                auto _f = create( path );
                if( _f == NULL ) {
                    throw util::Error::FS_ERROR;
                }
                fclose( _f );
            } else {
                throw util::Error::FS_ERROR;
            }
        }

        file = open( path );

        if( file == NULL ) {
            throw util::Error::FS_ERROR;
        }

        auto length = strlen( path );
        filePath = new char[length+1]{};
        memcpy( filePath, path, length );
    }

    File::~File() {
        if( file != NULL ) {
            fclose( file );
        }

        if( filePath != nullptr ) {
            delete[] filePath;
        }
    }

    bool File::exists( const char *path ) {
        auto *_f = fopen( path, "r" );

        if( _f == nullptr ) {
            return false;
        }

        fclose( _f );

        return true;
    }

    FILE *File::create( const char *path ) {
        return fopen( path, "w" );
    }

    FILE *File::open( const char *path ) {
        return fopen( path, "r+" );
    }

    void File::move( unsigned long int offset ) {
        if( fseek( file, offset, SEEK_SET ) != 0 ) {
            throw util::Error::FS_ERROR;
        }
    }

    void File::write( void *data, unsigned int length ) {
        if( fwrite( data, sizeof( char ), length, file ) != length ) {
            throw util::Error::FS_ERROR;
        }
    }

    void File::write( void *data, unsigned int length, unsigned long int offset ) {
        move( offset );

        return write( data, length );
    }

    unsigned int File::read( void *data, unsigned int length ) {
        return fread( data, sizeof( char ), length, file );
    }

    unsigned int File::read( void *data, unsigned int length, unsigned long int offset ) {
        move( offset );

        return read( data, length );
    }

    unsigned long int File::size() {
        auto *file = fopen( filePath, "r" );

        if( fseek( file, 0L, SEEK_END ) != 0 ) {
            throw util::Error::FS_ERROR;
        }

        auto size = ftell( file );
        fclose( file );

        return size;
    }

    void File::expand( unsigned int size ) {
        if( fseek( file, 0, SEEK_END ) != 0 ) {
            throw util::Error::FS_ERROR;
        }

        size += ftell( file );

        if( ftruncate( fileno( file ), size ) !=0 ) {
            throw util::Error::FS_ERROR;
        }
    }
}

#endif
