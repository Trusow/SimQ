#ifndef SIMQ_UTIL_ERROR 
#define SIMQ_UTIL_ERROR 

namespace simq::util {
    class Error {
        public:
            enum Err {
                NOT_FOUND,
                WRONG_PASSWORD,
                WRONG_PARAM,
                IS_EXISTS,
                ACCESS_DENY,
                UNKNOWN,
                FS_ERROR,
                DUPLICATE_GROUP,
                NOT_FOUND_GROUP,
                NOT_FOUND_SESSION,
                NOT_FOUND_CHANNEL,
                NOT_FOUND_CONSUMER,
                NOT_FOUND_PRODUCER,
                DUPLICATE_SESSION,
                DUPLICATE_CHANNEL,
                DUPLICATE_CONSUMER,
                DUPLICATE_PRODUCER,
            };
    };
}

#endif
