#ifndef SIMQ_CORE_SERVER_Q_BUFFER
#define SIMQ_CORE_SERVER_Q_BUFFER

#include <map>
#include <list>
#include <vector>
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
#include "../../../util/file.hpp"
#include "../../../util/messages.hpp"
#include "../../../util/error.h"
#include "../../../util/constants.h"
#include "../../../util/lock_atomic.hpp"

namespace simq::core::server::q {
    class Buffer {
        private:
            std::unique_ptr<util::File> _file;
            const unsigned int MESSAGE_PACKET_SIZE = util::constants::MESSAGE_PACKET_SIZE;
            const unsigned int MIN_FILE_SIZE = 50 * MESSAGE_PACKET_SIZE;
            const unsigned int SIZE_ITEM_PACKET = 10'000;

            struct Item {
                unsigned int length;
                unsigned int recvLength;

                std::unique_ptr<std::unique_ptr<char[]>[]> buffer;
                std::unique_ptr<unsigned long int[]> fileOffsets;
            };

            unsigned int _fileFD = 0;


            std::shared_timed_mutex _mItems;
            std::atomic_uint _countItemsWrited {0};
            std::vector<std::unique_ptr<Item>> _items;

            unsigned int _countItemsPackets = 0;
            std::list<unsigned int> _freeIDs;
            unsigned int _uniqID = 0;

            unsigned int _getUniqID();
            void _freeUniqID( unsigned int id );

            std::mutex _mFile;
            std::list<unsigned long int> _freeFileOffsets;
            unsigned long int _fileOffset = 0;

            void _initFileSize();
            void _expandItems();
            void _expandFile();
            void _wait( std::atomic_uint &atom );

            unsigned int _getOffsetPage( unsigned int length );
            unsigned int _getOffsetInnerPage( unsigned int length, unsigned int offsetPage );
            unsigned long int _getOffsetFile(
                Item *item,
                unsigned int offsetPage,
                unsigned int offsetInnerPage
            );

            unsigned int _checkRSLength( int length );
            unsigned int _calculateCountPages( unsigned int length );

            unsigned int _recv( char *data, unsigned int recvLength, unsigned int fd );
            unsigned int _recvToBuffer( Item *item, unsigned int fd );
            unsigned int _recvToFile( Item *item, unsigned int fd );
            unsigned int _sendFromBuffer( Item *item, unsigned int fd, unsigned int offset );
            unsigned int _sendFromFile( Item *item, unsigned int fd, unsigned int offset );

            Item *_getItem( unsigned int id );
        public:
            Buffer( const char *path );

            unsigned int allocate( unsigned int length );
            unsigned int allocateOnDisk( unsigned int length );
            void free( unsigned int id );

            void write(
                unsigned int id,
                const char *data,
                unsigned int length,
                unsigned int offsetPage
            );

            unsigned int recv( unsigned int id, unsigned int fd );
            unsigned int send( unsigned int id, unsigned int fd, unsigned int offset );

            unsigned int getLength( unsigned int id );
    };

    Buffer::Buffer( const char *path ) {
        _file = std::make_unique<util::File>( path, true );

        _fileFD = _file->fd();

        _initFileSize();
        _expandItems();
    }

    void Buffer::_wait( std::atomic_uint &atom ) {
        while( atom ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    unsigned int Buffer::_checkRSLength( int length ) {
        if( length == -1 ) {
            if( errno != EAGAIN ) {
                throw util::Error::SOCKET;
            }

            return 0;
        }

        return length;
    }

    unsigned int Buffer::_calculateCountPages( unsigned int length ) {
        auto count = length / MESSAGE_PACKET_SIZE;
        count += length - count * MESSAGE_PACKET_SIZE == 0 ? 0 : 1;

        return count;
    }

    unsigned int Buffer::_getOffsetPage( unsigned int length ) {
        return length / MESSAGE_PACKET_SIZE;
    }

    unsigned int Buffer::_getOffsetInnerPage( unsigned int length, unsigned int offsetPage ) {
        return length - offsetPage * MESSAGE_PACKET_SIZE;
    }

    unsigned long int Buffer::_getOffsetFile(
        Item *item,
        unsigned int offsetPage,
        unsigned int offsetInnerPage
    ) {
        return item->fileOffsets[offsetPage] * MESSAGE_PACKET_SIZE + offsetInnerPage;
    }

    unsigned int Buffer::allocateOnDisk( unsigned int length ) {
        util::LockAtomic lockAtomicGroup( _countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( _mItems );

        auto id = _getUniqID();

        auto item = std::make_unique<Item>();
        item->length = length;
        item->fileOffsets = std::make_unique<unsigned long int[]>( _calculateCountPages( length ) );
        _items[id] = std::move( item );

        return id;
    }

    unsigned int Buffer::allocate( unsigned int length ) {
        util::LockAtomic lockAtomicGroup( _countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( _mItems );

        auto id = _getUniqID();

        auto item = std::make_unique<Item>();
        item->length = length;
        item->buffer = std::make_unique<std::unique_ptr<char[]>[]>( _calculateCountPages( length ) );
        _items[id] = std::move( item );

        return id;
    }

    Buffer::Item *Buffer::_getItem( unsigned int id ) {
        if( id > _uniqID ) {
            return nullptr;
        }

        return _items[id].get();
    }

    unsigned int Buffer::getLength( unsigned int id ) {
        _wait( _countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( _mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return 0;
        }

        return item->length;
    }

    void Buffer::free( unsigned int id ) {
        util::LockAtomic lockAtomicGroup( _countItemsWrited );
        std::lock_guard<std::shared_timed_mutex> lockItems( _mItems );

        auto item = _getItem( id );
        
        if( item == nullptr ) {
            return;
        }

        if( item->fileOffsets != nullptr ) {
            std::lock_guard<std::mutex> lockFile( _mFile );
            auto countPages = _calculateCountPages( item->length );

            for( int i = 0; i < countPages; i++ ) {
                _freeFileOffsets.push_front( item->fileOffsets[i] );
            }

            item->fileOffsets.reset();
        }

        _items[id].reset();
        _freeUniqID( id );
    }

    unsigned int Buffer::_recv( char *data, unsigned int recvLength, unsigned int fd ) {
        auto length = ::recv( fd, data, recvLength, MSG_NOSIGNAL );

        return _checkRSLength( length );
    }

    unsigned int Buffer::_recvToBuffer( Item *item, unsigned int fd ) {
        auto recvLength = util::Messages::getResiduePart( item->length, item->recvLength );

        auto offsetPage = _getOffsetPage( item->recvLength );
        auto offsetInnerPage = _getOffsetInnerPage( item->recvLength, offsetPage );

        if( item->recvLength % MESSAGE_PACKET_SIZE == 0 ) {
            auto residue = util::Messages::getResiduePart( item->length, item->recvLength );
            if( residue > MESSAGE_PACKET_SIZE ) {
                item->buffer[offsetPage] = std::make_unique<char[]>( MESSAGE_PACKET_SIZE );
            } else {
                item->buffer[offsetPage] = std::make_unique<char[]>( residue );
            }
        }

        auto data = &item->buffer[offsetPage][offsetInnerPage];

        auto length = _recv( data, recvLength, fd );
        item->recvLength += length;

        return length;
    }

    unsigned int Buffer::_recvToFile( Item *item, unsigned int fd ) {
        auto recvLength = util::Messages::getResiduePart( item->length, item->recvLength );

        auto offsetPage = _getOffsetPage( item->recvLength );
        auto offsetInnerPage = _getOffsetInnerPage( item->recvLength, offsetPage );

        if( item->recvLength % MESSAGE_PACKET_SIZE == 0 ) {
            std::lock_guard<std::mutex> lockFile( _mFile );

            if( _freeFileOffsets.empty() ) {
                _expandFile();
            }

            item->fileOffsets[offsetPage] = _freeFileOffsets.front();
            _freeFileOffsets.pop_front();
        }

        char data[MESSAGE_PACKET_SIZE];

        auto length = _recv( data, recvLength, fd );

        if( length == 0 ) {
            return 0;
        }

        auto fileOffset = _getOffsetFile( item, offsetPage, offsetInnerPage );

        item->recvLength += length;

        _file->write(
            data,
            length,
            fileOffset
        );

        return length;
    }

    unsigned int Buffer::_sendFromBuffer( Item *item, unsigned int fd, unsigned int offset ) {
        auto sendLength = util::Messages::getResiduePart( item->length, offset );

        auto offsetPage = _getOffsetPage( offset );
        auto offsetInnerPage = _getOffsetInnerPage( offset, offsetPage );

        auto data = &item->buffer[offsetPage][offsetInnerPage];
        auto length = ::send( fd, data, sendLength, MSG_NOSIGNAL );

        return _checkRSLength( length );
    }

    unsigned int Buffer::_sendFromFile( Item *item, unsigned int fd, unsigned int offset ) {
        auto sendLength = util::Messages::getResiduePart( item->length, offset );

        auto offsetPage = _getOffsetPage( offset );
        auto offsetInnerPage = _getOffsetInnerPage( offset, offsetPage );
        auto fileOffset = _getOffsetFile( item, offsetPage, offsetInnerPage );

        auto length = ::sendfile( fd, _fileFD, (long *)&fileOffset, sendLength );

        return _checkRSLength( length );
    }


    unsigned int Buffer::recv( unsigned int id, unsigned int fd ) {
        _wait( _countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( _mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return 0;
        }

        if( item->buffer.get() ) {
            return _recvToBuffer( item, fd );
        }

        return _recvToFile( item, fd );
    }

    unsigned int Buffer::send( unsigned int id, unsigned int fd, unsigned int offset ) {
        _wait( _countItemsWrited );
        std::shared_lock<std::shared_timed_mutex> lockItems( _mItems );

        auto item = _getItem( id );

        if( item == nullptr ) {
            return 0;
        }

        if( item->buffer.get() ) {
            return _sendFromBuffer( item, fd, offset );
        }

        return _sendFromFile( item, fd, offset );
    }

    unsigned int Buffer::_getUniqID() {
        if( !_freeIDs.empty() ) {
            auto id = _freeIDs.front();
            _freeIDs.pop_front();
            return id;
        }

        _uniqID++;

        if( _uniqID % SIZE_ITEM_PACKET == 0 ) {
            _expandItems();
        }

        return _uniqID;
    }

    void Buffer::_freeUniqID( unsigned int id ) {
        _freeIDs.push_back( id );
    }

    void Buffer::_expandItems() {
        _countItemsPackets++;
        _items.resize( _items.size() + SIZE_ITEM_PACKET );
    }

    void Buffer::_expandFile() {
        _fileOffset += MIN_FILE_SIZE / MESSAGE_PACKET_SIZE;
        _file->expand( MIN_FILE_SIZE );
        for( unsigned int i = _fileOffset; i < _fileOffset + MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
            _freeFileOffsets.push_back( i );
        }
    }

    void Buffer::_initFileSize() {
        auto size = _file->size();
        _fileOffset = MIN_FILE_SIZE / MESSAGE_PACKET_SIZE;

        if( size == 0 ) {
            _file->expand( MIN_FILE_SIZE );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                _freeFileOffsets.push_back( i );
            }
        } else if( size % MESSAGE_PACKET_SIZE != 0 ) {
            auto expandSize =  MESSAGE_PACKET_SIZE - ( size - ( size / MESSAGE_PACKET_SIZE ) * MESSAGE_PACKET_SIZE );
            if( size + expandSize < MIN_FILE_SIZE ) {
                expandSize = MIN_FILE_SIZE - size;
                _file->expand( expandSize );

                for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                    _freeFileOffsets.push_back( i );
                }
            } else {
                _file->expand( expandSize );

                for( unsigned int i = 0; i < ( size + expandSize ) / MESSAGE_PACKET_SIZE; i++ ) {
                    _freeFileOffsets.push_back( i );
                }
                _fileOffset = ( size + expandSize ) / MESSAGE_PACKET_SIZE;
            }
        } else if( size < MIN_FILE_SIZE ) {
            auto expandSize = MIN_FILE_SIZE - size;
            _file->expand( expandSize );

            for( unsigned int i = 0; i < MIN_FILE_SIZE / MESSAGE_PACKET_SIZE; i++ ) {
                _freeFileOffsets.push_back( i );
            }
        } else {
            for( unsigned int i = 0; i < size / MESSAGE_PACKET_SIZE; i++ ) {
                _freeFileOffsets.push_back( i );
            }
            _fileOffset = size / MESSAGE_PACKET_SIZE;
        }
    }
}

#endif
