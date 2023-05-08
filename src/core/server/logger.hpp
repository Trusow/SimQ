#ifndef SIMQ_CORE_SERVER_LOGGER
#define SIMQ_CORE_SERVER_LOGGER

#include <list>
#include <string>
#include "../../util/logger.hpp"
#include "../../util/error.h"
#include "../../util/ip.hpp"
#include "../../util/types.h"

namespace simq::core::server {
    class Logger {
        public:
            enum Operation {
                OP_INITIALIZATION_SETTINGS,
                OP_INITIALIZATION_GROUP,
                OP_INITIALIZATION_CHANNEL,
                OP_INITIALIZATION_CONSUMER,
                OP_INITIALIZATION_PRODUCER,

                OP_START_SERVER,
     
                OP_ADD_GROUP,
                OP_UPDATE_GROUP_PASSWORD,
                OP_REMOVE_GROUP,

                OP_ADD_CHANNEL,
                OP_UPDATE_CHANNEL_LIMIT_MESSAGES,
                OP_REMOVE_CHANNEL,

                OP_ADD_CONSUMER,
                OP_UPDATE_CONSUMER_PASSWORD,
                OP_REMOVE_CONSUMER,

                OP_ADD_PRODUCER,
                OP_UPDATE_PRODUCER_PASSWORD,
                OP_REMOVE_PRODUCER,
            };

            struct Detail {
                std::string name;
                std::string value;
            };
        private:
            struct Row {
                std::string type;
                std::string initiator;
                std::string ip;
                std::string values;
            };

            static void _getOperation( Operation operation, std::string &str );
            static void _getInitiator(
                util::types::Initiator initiator,
                const char *group,
                const char *channel,
                const char *login,
                std::string &str
            );
            static void _getValues(
                std::list<Detail> details,
                std::string &str
            );
        public:

            static void addItemToDetails( std::list<Detail> &list, const char *name, const char *value );

            static void fail(
                Operation operation,
                util::Error::Err error,
                unsigned int ip,
                std::list<Detail> &details,
                util::types::Initiator initiator = util::types::Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
     
            static void success(
                Operation operation,
                unsigned int ip,
                std::list<Detail> &details,
                util::types::Initiator initiator = util::types::Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
    };

    void Logger::addItemToDetails( std::list<Detail> &list, const char *name, const char *value ) {
        Detail item; 
        item.name = name;
        item.value = value;
        list.push_back( item );
    }

    void Logger::_getOperation( Operation operation, std::string &str ) {
        switch( operation ) {
            case OP_INITIALIZATION_SETTINGS:
                str = "Initialization settings";
                break;
            case OP_INITIALIZATION_GROUP:
                str = "Initialization group";
                break;
            case OP_INITIALIZATION_CHANNEL:
                str = "Initialization channel";
                break;
            case OP_INITIALIZATION_CONSUMER:
                str = "Initialization consumer";
                break;
            case OP_INITIALIZATION_PRODUCER:
                str = "Initialization producer";
                break;
            case OP_START_SERVER:
                str = "Start server";
                break;
            case OP_ADD_GROUP:
                str = "Add group";
                break;
            case OP_UPDATE_GROUP_PASSWORD:
                str = "Update group password";
                break;
            case OP_REMOVE_GROUP:
                str = "Remove group";
                break;
            case OP_ADD_CHANNEL:
                str = "Add channel";
                break;
            case OP_UPDATE_CHANNEL_LIMIT_MESSAGES:
                str = "Update channel limit messages";
                break;
            case OP_REMOVE_CHANNEL:
                str = "Remove channel";
                break;
            case OP_ADD_CONSUMER:
                str = "Add consumer";
                break;
            case OP_UPDATE_CONSUMER_PASSWORD:
                str = "Update consumer password";
                break;
            case OP_REMOVE_CONSUMER:
                str = "Remove consumer";
            case OP_ADD_PRODUCER:
                str = "Add producer";
                break;
            case OP_UPDATE_PRODUCER_PASSWORD:
                str = "Update producer password";
                break;
            case OP_REMOVE_PRODUCER:
                str = "Remove producer";
        }
    }

    void Logger::_getInitiator(
        util::types::Initiator initiator,
        const char *group,
        const char *channel,
        const char *login,
        std::string &str
    ) {
        if( initiator == util::types::Initiator::I_GROUP ) {
            str = group;
        } else if( initiator == util::types::Initiator::I_CONSUMER || initiator == util::types::Initiator::I_PRODUCER ) {
            str = group;
            str += " / ";
            str = channel;
            str += " / ";
            str += initiator == util::types::Initiator::I_CONSUMER ? "consumer" : "producer";
            str += " / ";
            str = login;
        }
    }

    void Logger::_getValues(
        std::list<Detail> details,
        std::string &str
    ) {
        while( !details.empty() ) {
            auto v = details.front();
            str += v.name + " : " + v.value;
            details.pop_front();
            if( !details.empty() ) {
                str += ", ";
            }
        }
    }

    void Logger::fail(
        Operation operation,
        util::Error::Err error,
        unsigned int ip,
        std::list<Detail> &details,
        util::types::Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::string ceilType = "Fail";
        std::string ceilInitiator = "~";
        char ceilIP[16]{};
        std::string ceilValues = "";

        util::IP::toString( ip, ceilIP );

        std::string strOperation;
        std::string strErr = util::Error::getDescription( error );
        _getOperation( operation, strOperation );
        ceilType += " / ";
        ceilType += strOperation;
        ceilType += " / ";
        ceilType += strErr;

        _getInitiator( initiator, group, channel, login, ceilInitiator );

        _getValues( details, ceilValues );

        std::list<std::string> row;
        row.push_back( ceilType );
        row.push_back( ceilIP );
        row.push_back( ceilInitiator );
        row.push_back( ceilValues );

        util::Logger::danger( row );

    }

    void Logger::success(
        Operation operation,
        unsigned int ip,
        std::list<Detail> &details,
        util::types::Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::string ceilType = "Success";
        std::string ceilInitiator = "~";
        std::string ceilValues = "";

        char ceilIP[16]{};
        util::IP::toString( ip, ceilIP );

        std::string strOperation;
        std::string strErr;
        _getOperation( operation, strOperation );
        ceilType += " / ";
        ceilType += strOperation;

        _getInitiator( initiator, group, channel, login, ceilInitiator );

        _getValues( details, ceilValues );

        std::list<std::string> row;
        row.push_back( ceilType );
        row.push_back( ceilIP );
        row.push_back( ceilInitiator );
        row.push_back( ceilValues );

        util::Logger::info( row );

    }
}

#endif
 
