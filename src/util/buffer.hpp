#ifndef SIMQ_UTIL_BUFFER
#define SIMQ_UTIL_BUFFER

#include <map>
#include <queue>
#include <mutex>

namespace simq::util {
    class Buffer {
        private:
            const unsigned int LENGTH_PACKET_ON_DISK = 4096;
            const unsigned int LENGTH_PACKET_IN_MEMORY = 1024;

            struct Item {
                unsigned int lengthEnd;
                unsigned int length;
                char **buffer;
                unsigned long int *itemsDisk;
            };


            std::mutex mGenID;
            std::map<unsigned int, Item> items;
            std::queue<unsigned int> freeIDs;
            unsigned int uniqID = 0;

            unsigned int getUniqID();
            void freeUniqID( unsigned int id );

            std::mutex mFile;
            std::map<unsigned long int, Item> freeFileOffsets;
            unsigned long int fileOffset = 0;

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
}

#endif
