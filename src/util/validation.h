#ifndef SIMQ_UTIL_VALIDATION 
#define SIMQ_UTIL_VALIDATION 

namespace simq::util {
    class Validation {
        private:
            static bool isName( const char *name, unsigned int length );
        public:
            static bool isIPv4( const char *ip );
            static bool isGroupName( const char *name );
            static bool isChannelName( const char *name );
            static bool isConsumerName( const char *name );
            static bool isProducerName( const char *name );
    };
}

#endif
