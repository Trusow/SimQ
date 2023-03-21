#ifndef SIMQ_UTIL_BUFFER
#define SIMQ_UTIL_BUFFER

#include <map>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include "fs.hpp"
#include "error.h"
#include "lock_atomic.hpp"

namespace simq::util {
    class Buffer {
        private:
            const unsigned int LENGTH_PACKET_ON_DISK = 4096;
            const unsigned int MIN_FILE_SIZE = 50 * LENGTH_PACKET_ON_DISK;
            const unsigned int SIZE_ITEM_PACKET = 10'000;

            struct Item {
                unsigned int lengthEnd;
                unsigned int length;
                unsigned int packetSize;
                char **buffer;
                unsigned long int *itemsDisk;
            };

            FILE *file = nullptr;


            std::shared_timed_mutex mItems;
            std::atomic_uint countItemsWrited {0};
            Item ***items = nullptr;
            unsigned int countItemsPackets = 0;
            std::queue<unsigned int> freeIDs;
            unsigned int uniqID = 0;

            unsigned int getUniqID();
            void freeUniqID( unsigned int id );

            std::mutex mFile;
            std::queue<unsigned long int> freeFileOffsets;
            unsigned long int fileOffset = 0;

            void initFileSize();
            void initItems();
            void expandItems();
            void wait( std::atomic_uint &atom );
            void _writeBuffer( Item *item, const char *data, unsigned int length );
            void _writeFile( Item *item, const char *data, unsigned int length );
            char *_readBuffer( Item *item, char *data, unsigned int length, unsigned int offset );
            char *_readFile( Item *item, char *data, unsigned int length, unsigned int offset );

        public:
            Buffer( const char *path );
            ~Buffer();

            unsigned int allocate( unsigned int packetSize = 1024 );
            unsigned int allocateOnDisk();
            void free( unsigned int id );

            void write( unsigned int id, const char *data, unsigned int length );
            unsigned int getLength( unsigned int id );
            void read( unsigned int id, char *data, unsigned int length, unsigned int offset = 0 );
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
        initItems();
    }

    Buffer::~Buffer() {
        if( file ) {
            FS::closeFile( file );
        }

        for( int i = 0; i < countItemsPackets; i++ ) {
            for( int b = 0; b < SIZE_ITEM_PACKET; b++ ) {
                if( items[i][b] != nullptr ) {
                    if( items[i][b]->buffer != nullptr ) {
                        for( int c = 0; c < items[i][b]->length; c++ ) {
                            delete[] items[i][b]->buffer[c];
                        }
                        delete[] items[i][b]->buffer;
                    } else {
                        delete[] items[i][b]->itemsDisk;
                    }
                    delete items[i][b];
                }
            }
            delete[] items[i];
        }

        delete[] items;
    }

    void Buffer::wait( std::atomic_uint &atom ) {
        while( atom ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    unsigned int Buffer::allocate( unsigned int packetSize ) {
        if( packetSize == 0 ) {
            throw util::Error::WRONG_PARAM;
        }

        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( mItems );

        auto id = getUniqID();

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;


        auto item = new Item{};
        item->length++;
        item->buffer = new char*[item->length]{};
        item->buffer[0] = new char[packetSize]{};
        item->packetSize = packetSize;
        items[offsetPacket][offset] = item;

        return id;
    }

    unsigned int Buffer::getLength( unsigned int id ) {
        if( id > uniqID ) {
            return 0;
        }

        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( mItems );

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;
        auto item = items[offsetPacket][offset];

        return item->packetSize * ( item->length - 1 ) + item->lengthEnd;
    }

    void Buffer::free( unsigned int id ) {
        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( mItems );

        if( id > uniqID ) {
            return;
        }

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;
        auto item = items[offsetPacket][offset];

        if( item == nullptr ) {
            return;
        }

        if( item->buffer ) {
            for( int i = 0; i < item->length; i++ ) {
                delete[] item->buffer[i];
            }
            delete[] item->buffer;
        } else {
            delete[] item->itemsDisk;
        }

        delete item;
        items[offsetPacket][offset] = nullptr;
        freeUniqID( id );
    }

    char *Buffer::_readFile( Item *item, char *data, unsigned int length, unsigned int offset ) {
        return nullptr;
    }

    char *Buffer::_readBuffer( Item *item, char *data, unsigned int length, unsigned int offset ) {

        const unsigned int fullLength = item->packetSize * ( item->length - 1 ) + item->lengthEnd;

        if( offset + length > fullLength ) {
            return nullptr;
        }

        const unsigned int packetSize = item->packetSize;

        unsigned int pageStart = offset / packetSize;
        unsigned int sizeStart = offset - pageStart * packetSize;
        unsigned int acc = 0;

        memcpy( data, &item->buffer[pageStart][sizeStart], packetSize - sizeStart );
        acc += packetSize - sizeStart;

        unsigned int sizes = ( length - ( packetSize - sizeStart ) ) / packetSize;
        for( unsigned int i = 0; i < sizes; i++ ) {
            pageStart++;
            memcpy( &data[acc], &item->buffer[pageStart][0], packetSize );
            acc += packetSize;
        }

        const unsigned int residue = sizes * packetSize + ( packetSize - sizeStart );

        if( residue < length ) {
            pageStart++;
            memcpy( &data[acc], &item->buffer[pageStart][0], length - residue );
        }

        return data;
    }

    void Buffer::_writeBuffer( Item *item, const char *data, unsigned int length ) {
        const unsigned int packetSize = item->packetSize;
        const unsigned int writeSize = item->lengthEnd;
     
        unsigned int sizes = 0;
        unsigned int prevFreeSize = 0;
        unsigned int afterFreeSize = 0;

        if( writeSize + length <= packetSize ) {
            prevFreeSize = length;
        } else {
            prevFreeSize = packetSize - writeSize;
            sizes = ( length - prevFreeSize ) / packetSize + ( ( length - prevFreeSize ) % packetSize != 0 );
            afterFreeSize = length - prevFreeSize - ( sizes - 1 ) * packetSize;
        }

        if( prevFreeSize ) {
            memcpy( &item->buffer[item->length-1][item->lengthEnd], data, prevFreeSize );
            item->lengthEnd += prevFreeSize;
        }


        if( sizes ) {
            auto tmpBuffer = new char*[sizes+item->length]{};
            memcpy( tmpBuffer, item->buffer, item->length * sizeof( char * ) );

            delete[] item->buffer;
            item->buffer = tmpBuffer;

            for( unsigned int i = 0; i < sizes-1; i++ ) {
                item->length++;
                item->buffer[item->length-1] = new char[packetSize]{};
                memcpy( &item->buffer[item->length-1][0], &data[prevFreeSize + ( i * packetSize )], packetSize );
            }
        }


        if( afterFreeSize ) {
            item->length++;
            item->buffer[item->length-1] = new char[packetSize]{};
            item->lengthEnd = afterFreeSize;
            memcpy( &item->buffer[item->length-1][0], &data[length-afterFreeSize], afterFreeSize );
        }

    }

    void Buffer::_writeFile( Item *item, const char *data, unsigned int length ) {
        std::lock_guard<std::mutex> lock( mFile );
    }

    void Buffer::write( unsigned int id, const char *data, unsigned int length ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( mItems );

        if( id > uniqID ) {
            return;
        }

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;
        auto item = items[offsetPacket][offset];

        if( item == nullptr ) {
            return;
        }

        if( item->buffer ) {
            _writeBuffer( item, data, length );
        } else {
            _writeFile( item, data, length );
        }
    }

    void Buffer::read( unsigned int id, char *data, unsigned int length, unsigned int offset ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lock( mItems );

        if( id > uniqID ) {
            return;
        }

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto _offset = id - offsetPacket * SIZE_ITEM_PACKET;
        auto item = items[offsetPacket][_offset];

        if( item == nullptr ) {
            return;
        }


        if( item->buffer ) {
            _readBuffer( item, data, length, offset );
        } else {
            _readFile( item, data, length, offset );
        }
    }

    unsigned int Buffer::getUniqID() {

        if( !freeIDs.empty() ) {
            auto id = freeIDs.back();
            freeIDs.pop();
            return id;
        } else {
            uniqID++;
            if( uniqID % SIZE_ITEM_PACKET == 0 ) {
                expandItems();
            }
            return uniqID;
        }
    }

    void Buffer::freeUniqID( unsigned int id ) {
        freeIDs.push( id );
    }

    void Buffer::expandItems() {
        countItemsPackets++;
        auto tmpItems = new Item **[countItemsPackets]{};
        memcpy( tmpItems, items, sizeof( Item ** ) * ( countItemsPackets - 1 ) );
        delete items;
        items = tmpItems;
        items[countItemsPackets-1] = new Item*[SIZE_ITEM_PACKET]{};
    }

    void Buffer::initItems() {
        countItemsPackets++;
        items = new Item **[countItemsPackets]{};
        items[0] = new Item*[SIZE_ITEM_PACKET]{};
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
