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

            struct WRData {
                unsigned int startPartOffset;
                unsigned int startOffset;
                unsigned int startSize;
                unsigned int sizes;
                unsigned int endSize;
            };

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
            void expandFile();
            void wait( std::atomic_uint &atom );

            void _calculateWriteData(
                unsigned int fullLength,
                unsigned int partSize,
                unsigned int addLength,
                WRData &data
            );
            void _calculateReadData(
                unsigned int fullLength,
                unsigned int partSize,
                unsigned int offset,
                unsigned int readLength,
                WRData &data
            );

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

    unsigned int Buffer::allocateOnDisk() {
        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lock( mItems );

        auto id = getUniqID();

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;

        auto item = new Item{};
        item->packetSize = LENGTH_PACKET_ON_DISK;
        item->length++;
        item->itemsDisk = new unsigned long int[1]{};

        items[offsetPacket][offset] = item;

        std::lock_guard<std::mutex> lockFile( mFile );

        if( freeFileOffsets.empty() ) {
            expandFile();
        }

        item->itemsDisk[0] = freeFileOffsets.front();
        freeFileOffsets.pop();

        return id;
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
            std::lock_guard<std::mutex> lockFile( mFile );

            for( int i = 0; i < item->length; i++ ) {
                freeFileOffsets.push( item->itemsDisk[i] );
            }

            delete[] item->itemsDisk;
        }

        delete item;
        items[offsetPacket][offset] = nullptr;
        freeUniqID( id );
    }

    void Buffer::_calculateWriteData(
        unsigned int fullLength,
        unsigned int partSize,
        unsigned int addLength,
        WRData &data
    ) {
        unsigned int ceilFullLength = fullLength / partSize * partSize + ( fullLength % partSize == 0 ? 0 : partSize );
     
        if( ceilFullLength == 0 ) {
            ceilFullLength = partSize;
        }
     
        data.startPartOffset = ceilFullLength / partSize;
        if( data.startPartOffset != 0 ) {
            data.startPartOffset--;
        }
        data.startOffset = fullLength - data.startPartOffset * partSize;
        if( data.startOffset == partSize ) {
            data.startOffset = 0;
        }
     
        if( fullLength + addLength < ceilFullLength ) {
            data.sizes = 0;
            data.endSize = 0;
            data.startSize = addLength;
        } else {
            data.startSize = ceilFullLength - fullLength;
            addLength -= data.startSize;
            data.sizes = addLength / partSize;
            addLength -= data.sizes * partSize;
            data.endSize = addLength;
            if( data.endSize ) {
                data.sizes++;
            }
        }
    }
     
    void Buffer::_calculateReadData(
        unsigned int fullLength,
        unsigned int partSize,
        unsigned int offset,
        unsigned int readLength,
        WRData &data
    ) {
        if( offset + readLength > fullLength ) {
            return;
        }
     
        data.startPartOffset = offset / partSize;
        data.startOffset = offset - data.startPartOffset * partSize;
     
        if( partSize - data.startOffset > readLength ) {
            data.startSize = readLength;
            data.sizes = 0;
            data.endSize = 0;
        } else {
            data.startSize = partSize - data.startOffset;
            readLength -= data.startSize;
            data.sizes = readLength / partSize;
            data.endSize = readLength - data.sizes * partSize;
            if( data.endSize ) {
                data.sizes++;
            }
        }
    }

    char *Buffer::_readFile( Item *item, char *data, unsigned int length, unsigned int offset ) {
        std::lock_guard<std::mutex> lock( mFile );

        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateReadData( fullLength, packetSize, offset, length, wrData );

        if( wrData.startSize ) {
            FS::readFile(
                file,
                item->itemsDisk[wrData.startPartOffset] * packetSize + wrData.startOffset,
                wrData.startSize,
                (void *)data
            );
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            wrData.startPartOffset++;
            FS::readFile(
                file,
                item->itemsDisk[wrData.startPartOffset] * packetSize,
                i == wrData.sizes - 1 ? wrData.startSize : packetSize,
                (void *)&data[wrData.startSize + i * packetSize]
            );
        }

        return data;
    }

    void Buffer::_writeFile( Item *item, const char *data, unsigned int length ) {
        std::lock_guard<std::mutex> lock( mFile );


        const unsigned int packetSize = LENGTH_PACKET_ON_DISK;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );

        if( wrData.startSize ) {
            FS::writeFile(
                file,
                item->itemsDisk[wrData.startPartOffset] * packetSize + wrData.startOffset,
                wrData.startSize,
                (void *)data
            );
        }

        if( wrData.sizes ) {
            auto tmpItemsDisk = new unsigned long int[item->length + wrData.sizes]{};
            memcpy( tmpItemsDisk, item->itemsDisk, item->length * sizeof( unsigned long int ) );

            delete[] item->itemsDisk;
            item->itemsDisk = tmpItemsDisk;
            item->lengthEnd = wrData.endSize;
        } else {
            item->lengthEnd += wrData.startSize;
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            if( freeFileOffsets.empty() ) {
                expandFile();
            }
            wrData.startPartOffset++;
            item->length++;
            item->itemsDisk[wrData.startPartOffset] = freeFileOffsets.front();
            freeFileOffsets.pop();

            FS::writeFile(
                file,
                item->itemsDisk[wrData.startPartOffset] * packetSize,
                i == wrData.sizes - 1 ? wrData.endSize : packetSize,
                (void *)&data[wrData.startSize + i * packetSize]
            );
        }
    }

    char *Buffer::_readBuffer( Item *item, char *data, unsigned int length, unsigned int offset ) {
        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateReadData( fullLength, packetSize, offset, length, wrData );

        if( wrData.startSize ) {
            memcpy(
                data,
                &item->buffer[wrData.startPartOffset][wrData.startOffset],
                wrData.startSize
            );
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            wrData.startPartOffset++;
            memcpy(
                &data[wrData.startSize + i * packetSize],
                &item->buffer[wrData.startPartOffset][0],
                i == wrData.sizes - 1 ? wrData.endSize : packetSize
            );
        }

        return data;
    }

    void Buffer::_writeBuffer( Item *item, const char *data, unsigned int length ) {
        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );

        if( wrData.startSize ) {
            memcpy(
                &item->buffer[wrData.startPartOffset][wrData.startOffset],
                data,
                wrData.startSize
            );
        }


        if( wrData.sizes ) {
            auto tmpBuffer = new char*[wrData.sizes+item->length]{};
            memcpy( tmpBuffer, item->buffer, item->length * sizeof( char * ) );

            delete[] item->buffer;
            item->buffer = tmpBuffer;
            item->lengthEnd = wrData.endSize;
        } else {
            item->lengthEnd = wrData.startSize;
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            wrData.startPartOffset++;
            item->length++;
            item->buffer[wrData.startPartOffset] = new char[packetSize]{};
            memcpy(
                &item->buffer[wrData.startPartOffset][0],
                &data[wrData.startSize + packetSize * i],
                i == wrData.sizes - 1 ? wrData.endSize : packetSize
            );
        }

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
            auto id = freeIDs.front();
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

    void Buffer::expandFile() {
        fileOffset += MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK;
        FS::expandFile( file, MIN_FILE_SIZE );
        for( unsigned int i = fileOffset; i < fileOffset + MIN_FILE_SIZE / LENGTH_PACKET_ON_DISK; i++ ) {
            freeFileOffsets.push( i );
        }
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
