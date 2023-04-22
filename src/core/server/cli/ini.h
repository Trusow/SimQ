#ifndef SIMQ_CORE_SERVER_CLI_INI
#define SIMQ_CORE_SERVER_CLI_INI

namespace simq::core::server::CLI::Ini {
    inline const char *defPrefix = "simq";

    inline const char *cmdAuth = "auth";
    inline const char *cmdLs = "ls";
    inline const char *cmdH = "h";
    inline const char *cmdCd = "cd";
    inline const char *cmdInfo = "info";
    inline const char *cmdSet = "set";
    inline const char *cmdPswd = "passwd";
    inline const char *cmdRemove = "rm";
    inline const char *cmdAdd = "add";

    inline const char *pathGroups = "groups";
    inline const char *pathSettings = "settings";
    inline const char *pathConsumers = "consumers";
    inline const char *pathProducers = "producers";

    inline const char *infoSettingsPort = "port";
    inline const char *infoSettingsCountThreads = "countThreads";
    inline const char *infoChMinMessageSize = "minMessageSize";
    inline const char *infoChMaxMessageSize = "maxMessageSize";
    inline const char *infoChMaxMessagesInMemory = "maxMessagesInMemory";
    inline const char *infoChMaxMessagesOnDisk = "maxMessagesOnDisk";
}

#endif
