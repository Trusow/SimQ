#ifndef SIMQ_UTIL_TYPES
#define SIMQ_UTIL_TYPES

namespace simq::util::types {
    struct ChannelLimitMessages {
        unsigned int minMessageSize;
        unsigned int maxMessageSize;
        unsigned int maxMessagesInMemory;
        unsigned int maxMessagesOnDisk;
    };

    enum Initiator {
        I_ROOT,
        I_GROUP,
        I_CONSUMER,
        I_PRODUCER,
    };
}

#endif
