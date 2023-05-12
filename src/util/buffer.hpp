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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <memory>
#include "file.hpp"
#include "error.h"
#include "constants.h"
#include "lock_atomic.hpp"

namespace simq::util {
    class Buffer {
        private:
            std::unique_ptr<File> file;
            const unsigned int MESSAGE_PACKET_SIZE = util::constants::MESSAGE_PACKET_SIZE;
            const unsigned int MIN_FILE_SIZE = 50 * MESSAGE_PACKET_SIZE;
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
                std::unique_ptr<std::unique_ptr<char[]>[]> buffer;
                std::unique_ptr<unsigned long int[]> fileOffsets;
            };

            unsigned int fileFD = 0;


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

            Item *_getItem( unsigned int id );
            void _writeBuffer( Item *item, const char *data, unsigned int length );
            void _writeFile( Item *item, const char *data, unsigned int length );
            char *_readBuffer( Item *item, char *data, unsigned int length, unsigned int offset );
            char *_readFile( Item *item, char *data, unsigned int length, unsigned int offset );
            unsigned int _recvToBuffer( Item *item, unsigned int fd, unsigned int length );
            unsigned int _recvToFile( Item *item, unsigned int fd, unsigned int length );
            unsigned int _sendFromBuffer(
                Item *item,
                unsigned int fd,
                unsigned int length,
                unsigned int offset
            );
            unsigned int _sendFromFile(
                Item *item,
                unsigned int fd,
                unsigned int length,
                unsigned int offset
            );

        public:
            Buffer( const char *path );
            ~Buffer();

            unsigned int allocate( unsigned int length, unsigned int packetSize = 1024 );
            unsigned int allocateOnDisk( unsigned int length );
            void free( unsigned int id );

            void write( unsigned int id, const char *data, unsigned int length );
            unsigned int getLength( unsigned int id );
            void read( unsigned int id, char *data, unsigned int length, unsigned int offset = 0 );
            unsigned int recv( unsigned int id, unsigned int fd, unsigned int length );
            unsigned int send( unsigned int id, unsigned int fd, unsigned int length, unsigned int offset = 0 );
    };

    Buffer::Buffer( const char *path ) {
        file = std::make_unique<File>( path, true );

        fileFD = file->fd();

        initFileSize();
        initItems();
    }

    Buffer::~Buffer() {

        for( int i = 0; i < countItemsPackets; i++ ) {
            for( int b = 0; b < SIZE_ITEM_PACKET; b++ ) {
                if( items[i][b] != nullptr ) {
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

    unsigned int Buffer::allocateOnDisk( unsigned int length ) {
        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( mItems );

        auto id = getUniqID();

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;
        auto l = length / MESSAGE_PACKET_SIZE;
        l += length - l * MESSAGE_PACKET_SIZE == 0 ? 0 : 1;

        auto item = new Item{};
        item->packetSize = MESSAGE_PACKET_SIZE;
        item->length++;
        item->fileOffsets = std::make_unique<unsigned long int[]>( l );

        items[offsetPacket][offset] = item;

        std::lock_guard<std::mutex> lockFile( mFile );

        if( freeFileOffsets.empty() ) {
            expandFile();
        }

        item->fileOffsets[0] = freeFileOffsets.front();
        freeFileOffsets.pop();

        return id;
    }

    unsigned int Buffer::allocate( unsigned int length, unsigned int packetSize ) {
        if( packetSize == 0 ) {
            throw util::Error::WRONG_PARAM;
        }

        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( mItems );

        auto id = getUniqID();

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;

        auto l = length / packetSize;
        l += length - l * packetSize == 0 ? 0 : 1;


        auto item = new Item{};
        item->length++;
        item->buffer = std::make_unique<std::unique_ptr<char[]>[]>( l );
        item->buffer[0] = std::make_unique<char[]>( packetSize );
        item->packetSize = packetSize;
        items[offsetPacket][offset] = item;

        return id;
    }

    Buffer::Item *Buffer::_getItem( unsigned int id ) {
        if( id > uniqID ) {
            return nullptr;
        }

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;

        return items[offsetPacket][offset];
    }

    unsigned int Buffer::getLength( unsigned int id ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );

        if( !item ) {
            return 0;
        }

        return item->packetSize * ( item->length - 1 ) + item->lengthEnd;
    }

    void Buffer::free( unsigned int id ) {
        util::LockAtomic lockAtomicGroup( countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );
        
        if( item == nullptr ) {
            return;
        }

        if( item->fileOffsets != nullptr ) {
            std::lock_guard<std::mutex> lockFile( mFile );

            for( int i = 0; i < item->length; i++ ) {
                freeFileOffsets.push( item->fileOffsets[i] );
            }

            item->fileOffsets.reset();
        }

        delete item;

        auto offsetPacket = id / SIZE_ITEM_PACKET;
        auto offset = id - offsetPacket * SIZE_ITEM_PACKET;

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

            if( fullLength % partSize == 0 ) {
                data.endSize = partSize;
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

            if( fullLength % partSize == 0 ) {
                data.endSize = partSize;
            }
        }
    }

    unsigned int Buffer::_sendFromFile(
        Item *item,
        unsigned int fd,
        unsigned int length,
        unsigned int offset
    ) {
        std::lock_guard<std::mutex> lockFile( mFile );

        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateReadData( fullLength, packetSize, offset, length, wrData );

        if( wrData.startSize ) {
            auto _offset = item->fileOffsets[wrData.startPartOffset] * packetSize + wrData.startOffset;
            auto size = ::sendfile(
                fd,
                fileFD,
                (long *)&_offset,
                wrData.startSize
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            return size;
        }

        if( wrData.sizes ) {
            wrData.startPartOffset++;
            auto _offset = item->fileOffsets[wrData.startPartOffset] * packetSize;
            auto size = ::sendfile(
                fd,
                fileFD,
                (long *)&_offset,
                1 == wrData.sizes ? wrData.startSize : packetSize
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            return size;
        }

        return 0;
    }

    char *Buffer::_readFile( Item *item, char *data, unsigned int length, unsigned int offset ) {
        std::lock_guard<std::mutex> lockFile( mFile );

        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateReadData( fullLength, packetSize, offset, length, wrData );

        if( wrData.startSize ) {
            file->read(
                (void *)data,
                wrData.startSize,
                item->fileOffsets[wrData.startPartOffset] * packetSize + wrData.startOffset
            );
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            wrData.startPartOffset++;
            file->read(
                (void *)&data[wrData.startSize + i * packetSize],
                i == wrData.sizes - 1 ? wrData.endSize : packetSize,
                item->fileOffsets[wrData.startPartOffset] * packetSize
            );
        }

        return data;
    }

    unsigned int Buffer::_recvToFile( Item *item, unsigned int fd, unsigned int length ) {
        std::lock_guard<std::mutex> lockFile( mFile );

        const unsigned int packetSize = MESSAGE_PACKET_SIZE;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );
        char data[MESSAGE_PACKET_SIZE];

        if( wrData.startSize ) {

            auto size = ::recv(
                fd,
                data,
                length,
                MSG_NOSIGNAL
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            file->write(
                (void *)data,
                size,
                item->fileOffsets[wrData.startPartOffset] * packetSize + wrData.startOffset
            );

            item->lengthEnd += size;

            return size;
        }

        if( wrData.sizes ) {
            item->length++;
            item->lengthEnd = 0;

            if( freeFileOffsets.empty() ) {
                expandFile();
            }

            wrData.startPartOffset++;
            item->fileOffsets[wrData.startPartOffset] = freeFileOffsets.front();
            freeFileOffsets.pop();

            auto size = ::recv(
                fd,
                data,
                1 == wrData.sizes ? wrData.endSize : packetSize,
                MSG_NOSIGNAL
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            file->write(
                (void *)data,
                size,
                item->fileOffsets[wrData.startPartOffset] * packetSize
            );

            item->lengthEnd = size;

            return size;
        }

        return 0;
    }

    void Buffer::_writeFile( Item *item, const char *data, unsigned int length ) {
        std::lock_guard<std::mutex> lockFile( mFile );


        const unsigned int packetSize = MESSAGE_PACKET_SIZE;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );

        if( wrData.startSize ) {
            file->write(
                (void *)data,
                wrData.startSize,
                item->fileOffsets[wrData.startPartOffset] * packetSize + wrData.startOffset
            );
        }

        if( wrData.sizes ) {
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
            item->fileOffsets[wrData.startPartOffset] = freeFileOffsets.front();
            freeFileOffsets.pop();

            file->write(
                (void *)&data[wrData.startSize + i * packetSize],
                i == wrData.sizes - 1 ? wrData.endSize : packetSize,
                item->fileOffsets[wrData.startPartOffset] * packetSize
            );
        }
    }

    unsigned int Buffer::_sendFromBuffer(
        Item *item,
        unsigned int fd,
        unsigned int length,
        unsigned int offset
    ) {
        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateReadData( fullLength, packetSize, offset, length, wrData );

        if( wrData.startSize ) {
            auto size = ::send(
                fd,
                &item->buffer[wrData.startPartOffset][wrData.startOffset],
                wrData.startSize,
                MSG_NOSIGNAL
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }
                
                return 0;
            }

            return size;
        }

        if( wrData.sizes ) {
            wrData.startPartOffset++;

            auto size = ::send(
                fd,
                &item->buffer[wrData.startPartOffset][0],
                1 == wrData.sizes ? wrData.endSize : packetSize,
                MSG_NOSIGNAL
            );

            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }
                
                return 0;
            }

            return size;
        }

        return 0;
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

    unsigned int Buffer::_recvToBuffer( Item *item, unsigned int fd, unsigned int length ) {
        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );

        if( wrData.startSize ) {
            auto size = ::recv(
                fd,
                &item->buffer[wrData.startPartOffset][wrData.startOffset],
                wrData.startSize,
                MSG_NOSIGNAL
            );
            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            item->lengthEnd += size;

            return size;
        }

        if( wrData.sizes ) {
            item->length++;
            item->lengthEnd = 0;

            auto size = ::recv(
                fd,
                &item->buffer[wrData.startPartOffset+1][0],
                1 == wrData.sizes ? wrData.endSize : packetSize,
                MSG_NOSIGNAL
            );
            if( size == -1 ) {
                if( errno != EAGAIN ) {
                    throw util::Error::SOCKET;
                }

                return 0;
            }

            item->lengthEnd = size;

            return size;
        }

        return 0;

    }

    void Buffer::_writeBuffer( Item *item, const char *data, unsigned int length ) {
        const unsigned int packetSize = item->packetSize;
        WRData wrData;
        unsigned int fullLength = item->length == 1 ? item->lengthEnd : item->length * packetSize + item->lengthEnd;
        _calculateWriteData( fullLength, packetSize, length, wrData );

        if( wrData.startSize ) {
            memcpy(
                &item->buffer[wrData.startPartOffset].get()[wrData.startOffset],
                data,
                wrData.startSize
            );
        }


        if( wrData.sizes ) {
            item->lengthEnd = wrData.endSize;
        } else {
            item->lengthEnd = wrData.startSize;
        }

        for( unsigned int i = 0; i < wrData.sizes; i++ ) {
            wrData.startPartOffset++;
            item->length++;
            item->buffer[wrData.startPartOffset] = std::make_unique<char[]>( packetSize );
            memcpy(
                item->buffer[wrData.startPartOffset].get(),
                &data[wrData.startSize + packetSize * i],
                i == wrData.sizes - 1 ? wrData.endSize : packetSize
            );
        }

    }


    void Buffer::write( unsigned int id, const char *data, unsigned int length ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return;
        }

        if( item->buffer.get() ) {
            _writeBuffer( item, data, length );
        } else {
            _writeFile( item, data, length );
        }
    }

    void Buffer::read( unsigned int id, char *data, unsigned int length, unsigned int offset ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return;
        }

        if( item->buffer.get() ) {
            _readBuffer( item, data, length, offset );
        } else {
            _readFile( item, data, length, offset );
        }
    }

    unsigned int Buffer::recv( unsigned int id, unsigned int fd, unsigned int length ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return 0;
        }

        if( item->buffer.get() ) {
            return _recvToBuffer( item, fd, length );
        } else {
            return _recvToFile( item, fd, length );
        }
    }

    unsigned int Buffer::send(
        unsigned int id,
        unsigned int fd,
        unsigned int length,
        unsigned int offset
    ) {
        wait( countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return 0;
        }

        if( item->buffer.get() ) {
            return _sendFromBuffer( item, fd, length, offset );
        } else {
            return _sendFromFile( item, fd, length, offset );
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
        fileOffset += MIN_FILE_SIZE / MESSAGE_PACKET_SIZE;
        file->expand( MIN_FILE_SIZE );
        for( unsigned int i = fileOffset; i < fileOffset + MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
            freeFileOffsets.push( i );
        }
    }

    void Buffer::initFileSize() {
        auto size = file->size();
        fileOffset = MIN_FILE_SIZE / MESSAGE_PACKET_SIZE;

        if( size == 0 ) {
            file->expand( MIN_FILE_SIZE );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                freeFileOffsets.push( i );
            }
        } else if( size % MESSAGE_PACKET_SIZE != 0 ) {
            auto expandSize =  MESSAGE_PACKET_SIZE - ( size - ( size / MESSAGE_PACKET_SIZE ) * MESSAGE_PACKET_SIZE );
            if( size + expandSize < MIN_FILE_SIZE ) {
                expandSize = MIN_FILE_SIZE - size;
                file->expand( expandSize );

                for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                    freeFileOffsets.push( i );
                }
            } else {
                file->expand( expandSize );

                for( unsigned int i = 0; i < ( size + expandSize ) / MESSAGE_PACKET_SIZE; i++ ) {
                    freeFileOffsets.push( i );
                }
                fileOffset = ( size + expandSize ) / MESSAGE_PACKET_SIZE;
            }
        } else if( size < MIN_FILE_SIZE ) {
            auto expandSize = MIN_FILE_SIZE - size;
            file->expand( expandSize );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                freeFileOffsets.push( i );
            }
        } else {
            for( unsigned int i = 0; i < size / MESSAGE_PACKET_SIZE; i++ ) {
                freeFileOffsets.push( i );
            }
            fileOffset = size / MESSAGE_PACKET_SIZE;
        }
    }
}

#endif
