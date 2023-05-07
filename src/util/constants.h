#ifndef SIMQ_UTIL_CONSTANTS
#define SIMQ_UTIL_CONSTANTS

#include <string>

namespace simq::util::constants {
    inline const unsigned int MESSAGE_PACKET_SIZE = 4096;
    inline const unsigned int DEFAULT_PORT = 4012;

    inline const char *PATH_DIR_GROUPS = "groups";
    inline const char *PATH_DIR_CHANNELS = "channels";
    inline const char *PATH_DIR_CONSUMERS = "consumers";
    inline const char *PATH_DIR_PRODUCERS = "producers";
    inline const char *PATH_FILE_PASSWORD = "password";
    inline const char *PATH_FILE_CHANNEL_LIMIT_MESSAGES = "limit-messages";
    inline const char *PATH_FILE_CHANNEL_DATA = "data";
    inline const char *PATH_DIR_SETTINGS = "settings";
    inline const char *PATH_FILE_SETTINGS = "settings";
    inline const char *PATH_DIR_CHANGES = "changes";

    inline void buildPathToDirSettings( std::string &str, const char *path ) {
        str = path;
        str += "/";
        str += PATH_DIR_SETTINGS;
    }

    inline void buildPathToSettings( std::string &str, const char *path ) {
        buildPathToDirSettings( str, path );

        str += "/";
        str += PATH_FILE_SETTINGS;
    }

    inline void buildPathToGroups( std::string &str, const char *path ) {
        str = path;
        str += "/";
        str += PATH_DIR_GROUPS;
    }

    inline void buildPathToGroup( std::string &str, const char *path, const char *group ) {
        buildPathToGroups( str, path );

        str += "/";
        str += group;
    }

    inline void buildPathToGroupPassword( std::string &str, const char *path, const char *group ) {
        buildPathToGroup( str, path, group );

        str += "/";
        str += PATH_FILE_PASSWORD;
    }

    inline void buildPathToChannel( std::string &str, const char *path, const char *group, const char *channel ) {
        buildPathToGroup( str, path, group );

        str += "/";
        str += channel;
    }

    inline void buildPathToChannelLimitMessages( std::string &str, const char *path, const char *group, const char *channel ) {
        buildPathToChannel( str, path, group, channel );

        str += "/";
        str += PATH_FILE_CHANNEL_LIMIT_MESSAGES;
    }

    inline void buildPathToChannelData( std::string &str, const char *path, const char *group, const char *channel ) {
        buildPathToChannel( str, path, group, channel );

        str += "/";
        str += PATH_FILE_CHANNEL_DATA;
    }

    inline void buildPathToProducers( std::string &str, const char *path, const char *group, const char *channel ) {
        buildPathToChannel( str, path, group, channel );

        str += "/";
        str += PATH_DIR_PRODUCERS;
    }

    inline void buildPathToProducer( std::string &str, const char *path, const char *group, const char *channel, const char *login ) {
        buildPathToProducers( str, path, group, channel );

        str += "/";
        str += login;
    }

    inline void buildPathToProducerPassword( std::string &str, const char *path, const char *group, const char *channel, const char *login ) {
        buildPathToProducer( str, path, group, channel, login );
        str += "/";
        str += PATH_FILE_PASSWORD;
    }

    inline void buildPathToConsumers( std::string &str, const char *path, const char *group, const char *channel ) {
        buildPathToChannel( str, path, group, channel );

        str += "/";
        str += PATH_DIR_CONSUMERS;
    }

    inline void buildPathToConsumer( std::string &str, const char *path, const char *group, const char *channel, const char *login ) {
        buildPathToConsumers( str, path, group, channel );

        str += "/";
        str += login;
    }

    inline void buildPathToConsumerPassword( std::string &str, const char *path, const char *group, const char *channel, const char *login ) {
        buildPathToConsumer( str, path, group, channel, login );
        str += "/";
        str += PATH_FILE_PASSWORD;
    }
}

#endif
 
