#ifndef SIMQ_UTIL_MESSAGES
#define SIMQ_UTIL_MESSAGES

#include "constants.h"

namespace simq::util {
    class Messages {
        private:
            const static unsigned int PACKET_SIZE = util::constants::MESSAGE_PACKET_SIZE;
        public:
            static unsigned int getResiduePart( unsigned int length, unsigned int rsLength );
    };

    unsigned int Messages::getResiduePart( unsigned int length, unsigned int rsLength ){
        auto residue = length - rsLength;

        if( residue < PACKET_SIZE ) {
            return residue;
        }

        auto countRSPackets =  rsLength / PACKET_SIZE;
        residue = rsLength - countRSPackets * PACKET_SIZE;
 
        return PACKET_SIZE - residue;
    }
}

#endif
 
