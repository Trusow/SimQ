#ifndef SIMQ_UTIL_TYPES
#define SIMQ_UTIL_TYPES

namespace simq::util::types {
    struct ChannelLimitMessages {
        unsigned int minMessageSize;
        unsigned int maxMessageSize;
        unsigned int maxMessagesInMemory;
        unsigned int maxMessagesOnDisk;
    };
}

#endif
