#ifndef SIMQ_UTIL_ERROR 
#define SIMQ_UTIL_ERROR 

namespace simq::util {
    class Error {
        public:
            enum Err {
                NOT_FOUND,
                WRONG_PASSWORD,
                WRONG_PARAM,
                WRONG_GROUP,
                WRONG_CHANNEL,
                WRONG_LOGIN,
                WRONG_MESSAGE_SIZE,
                WRONG_SETTINGS,
                WRONG_CHANNEL_LIMIT_MESSAGES,
                WRONG_CONSUMER,
                WRONG_PRODUCER,
                WRONG_UUID,
                WRONG_CMD,
                EXCEED_LIMIT,
                IS_EXISTS,
                ACCESS_DENY,
                UNKNOWN,
                FS_ERROR,
                DUPLICATE_GROUP,
                NOT_FOUND_GROUP,
                NOT_FOUND_CHANNEL,
                NOT_FOUND_CONSUMER,
                NOT_FOUND_PRODUCER,
                DUPLICATE_SESSION,
                DUPLICATE_CHANNEL,
                DUPLICATE_CONSUMER,
                DUPLICATE_PRODUCER,
                DUPLICATE_UUID,
                SOCKET,
            };

            static const char *getDescription( Err err ) {
                switch( err ) {
                    case DUPLICATE_UUID:
                        return "Duplicate UUID";
                    case NOT_FOUND:
                        return "Not found";
                    case WRONG_PASSWORD:
                        return "Wrong password";
                    case WRONG_PARAM:
                        return "Wrong params";
                    case WRONG_GROUP:
                        return "Wrong group";
                    case WRONG_CHANNEL:
                        return "Wrong channel";
                    case WRONG_LOGIN:
                        return "Wrong login";
                    case WRONG_MESSAGE_SIZE:
                        return "Wrong message size";
                    case WRONG_CHANNEL_LIMIT_MESSAGES:
                        return "Wrong channel limit messages";
                    case WRONG_CONSUMER:
                        return "Wrong consumer";
                    case WRONG_PRODUCER:
                        return "Wrong producer";
                    case WRONG_UUID:
                        return "Wrong UUID";
                    case WRONG_SETTINGS:
                        return "Wrong settings";
                    case WRONG_CMD:
                        return "Wrong command";
                    case EXCEED_LIMIT:
                        return "Exceed limit";
                    case IS_EXISTS:
                        return "Is exists";
                    case ACCESS_DENY:
                        return "Access deny";
                    case util::Error::FS_ERROR:
                        return "FS error";
                    case DUPLICATE_GROUP:
                        return "Duplicate group";
                    case NOT_FOUND_GROUP:
                        return "Not found group";
                    case NOT_FOUND_CHANNEL:
                        return "Not found channel";
                    case NOT_FOUND_CONSUMER:
                        return "Not found consumer";
                    case NOT_FOUND_PRODUCER:
                        return "Not found producer";
                    case DUPLICATE_SESSION:
                        return "Duplicate session";
                    case DUPLICATE_CHANNEL:
                        return "Duplicate channel";
                    case DUPLICATE_CONSUMER:
                        return "Duplicate consumer";
                    case DUPLICATE_PRODUCER:
                        return "Duplicate producer";
                    case SOCKET:
                        return "Connection error";
                    default:
                        return "Unknown";
                }
            }
    };
}

#endif
