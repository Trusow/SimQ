#ifndef SIMQ_UTIL_BUFFER
#define SIMQ_UTIL_BUFFER

namespace simq::util {
    class Buffer {
        private:
            const unsigned int LENGTH_PACKET = 4096;
            struct Item {
                unsigned int lengthEnd;
                unsigned int length;
                char **buffer;
                unsigned long int **itemsDisk;
            };

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
}

#endif
