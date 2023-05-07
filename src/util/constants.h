#ifndef SIMQ_UTIL_CONSTANTS
#define SIMQ_UTIL_CONSTANTS

namespace simq::util::constants {
    inline const unsigned int MESSAGE_PACKET_SIZE = 4096;

    inline const char *PATH_DIR_GROUPS = "groups";
    inline const char *PATH_DIR_CHANNELS = "channels";
    inline const char *PATH_DIR_CONSUMERS = "consumers";
    inline const char *PATH_DIR_PRODUCERS = "producers";
    inline const char *PATH_FILE_PASSWORD = "password";
    inline const char *PATH_FILE_CHANNEL_LIMIT_MESSAGES = "limit-messages";
    inline const char *PATH_DIR_SETTINGS = "settings";
    inline const char *PATH_FILE_SETTINGS = "settings";
    inline const char *PATH_DIR_CHANGES = "changes";
}

#endif
 
