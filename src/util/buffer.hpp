#ifndef SIMQ_UTIL_BUFFER
#define SIMQ_UTIL_BUFFER

#include <map>
#include <queue>
#include <mutex>
#include "fs.hpp"
#include "error.h"

namespace simq::util {
    class Buffer {
        private:
            const unsigned int LENGTH_PACKET_ON_DISK = 4096;
            const unsigned int LENGTH_PACKET_IN_MEMORY = 1024;
            const unsigned int MIN_FILE_SIZE = 50 * LENGTH_PACKET_ON_DISK;

            struct Item {
                unsigned int lengthEnd;
                unsigned int length;
                char **buffer;
                unsigned long int *itemsDisk;
            };

            FILE *file = nullptr;


            std::mutex mGenID;
            std::map<unsigned int, Item> items;
            std::queue<unsigned int> freeIDs;
            unsigned int uniqID = 0;

            unsigned int getUniqID();
            void freeUniqID( unsigned int id );

            std::mutex mFile;
            std::queue<unsigned long int> freeFileOffsets;
            unsigned long int fileOffset = 0;

            void initFileSize();

        public:
            Buffer( const char *path );
            ~Buffer();

            unsigned int allocate();
            unsigned int allocateOnDisk();
            void free( unsigned int id );

            void write( unsigned int id, const char *data, unsigned int length );
            unsigned int getLength( unsigned int id );
            char *read( unsigned int id, unsigned int offset, unsigned int length );
    };

    Buffer::Buffer( const char *path ) {
        if( !FS::fileExists( path ) ) {
            if( !FS::createFile( path ) ) {
                throw util::Error::FS_ERROR;
            }
        }

        file = FS::openFile( path );

        if( !file ) {
            throw util::Error::FS_ERROR;
        }

        initFileSize();
    }

    Buffer::~Buffer() {
        if( file ) {
            FS::closeFile( file );
        }
    }

    unsigned int Buffer::getUniqID() {
        std::lock_guard<std::mutex> lock( mGenID );

        if( !freeIDs.empty() ) {
            auto id = freeIDs.back();
            freeIDs.pop();
            return id;
        } else {
            uniqID++;
            return uniqID;
        }
    }

    void Buffer::freeUniqID( unsigned int id ) {
        std::lock_guard<std::mutex> lock( mGenID );
        freeIDs.push( id );
    }

    void Buffer::initFileSize() {
        auto size = FS::getFileSize( file );
        fileOffset = MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK;

        if( size == 0 ) {
            FS::expandFile( file, MIN_FILE_SIZE );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK; i++ ) {
                freeFileOffsets.push( i );
            }
        } else if( size % LENGTH_PACKET_ON_DISK != 0 ) {
            auto expandSize =  LENGTH_PACKET_ON_DISK - ( size - ( size / LENGTH_PACKET_ON_DISK ) * LENGTH_PACKET_ON_DISK );
            if( size + expandSize < MIN_FILE_SIZE ) {
                expandSize = MIN_FILE_SIZE - size;
                FS::expandFile( file, expandSize );

                for( unsigned int i = 0; i < MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK; i++ ) {
                    freeFileOffsets.push( i );
                }
            } else {
                FS::expandFile( file, expandSize );

                for( unsigned int i = 0; i < ( size + expandSize ) / LENGTH_PACKET_ON_DISK; i++ ) {
                    freeFileOffsets.push( i );
                }
                fileOffset = ( size + expandSize ) / LENGTH_PACKET_ON_DISK;
            }
        } else if( size < MIN_FILE_SIZE ) {
            auto expandSize = MIN_FILE_SIZE - size;
            FS::expandFile( file, expandSize );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK; i++ ) {
                freeFileOffsets.push( i );
            }
        } else {
            for( unsigned int i = 0; i < size / LENGTH_PACKET_ON_DISK; i++ ) {
                freeFileOffsets.push( i );
            }
            fileOffset = size / LENGTH_PACKET_ON_DISK;
        }
    }
}

#endif
