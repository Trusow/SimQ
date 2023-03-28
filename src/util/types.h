#ifndef SIMQ_UTIL_TYPES
#define SIMQ_UTIL_TYPES

namespace simq::util {
    class Types {
        public:
            struct ChannelSettings {
                unsigned int minMessageSize;
                unsigned int maxMessageSize;
                unsigned int maxMessagesInMemory;
                unsigned int maxMessagesOnDisk;
            };
    };
}

#endif
