#ifndef SIMQ_CORE_SERVER_LOGGER
#define SIMQ_CORE_SERVER_LOGGER

#include <list>
#include <string>
#include "../../util/logger.hpp"
#include "../../util/error.h"

namespace simq::core::server {
    class Logger {
        public:
            enum Operation {
                OP_INITIALIZATION_GROUP,
                OP_INITIALIZATION_CHANNEL,
                OP_INITIALIZATION_CONSUMER,
                OP_INITIALIZATION_PRODUCER,

                OP_START_SERVER,
     
                OP_CREATE_GROUP,
            };

            struct Detail {
                std::string name;
                std::string value;
            };

            enum Initiator {
                I_ROOT,
                I_GROUP,
                I_CONSUMER,
                I_PRODUCER,
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
                Initiator initiator,
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
            static void fail(
                Operation operation,
                util::Error::Err error,
                unsigned int ip,
                std::list<Detail> &details,
                Initiator initiator = Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
     
            static void success(
                Operation operation,
                unsigned int ip,
                std::list<Detail> &details,
                Initiator initiator = Initiator::I_ROOT,
                const char *group = nullptr,
                const char *channel = nullptr,
                const char *login = nullptr
            );
    };

    void Logger::_getOperation( Operation operation, std::string &str ) {
        switch( operation ) {
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
        }
    }

    void Logger::_getInitiator(
        Initiator initiator,
        const char *group,
        const char *channel,
        const char *login,
        std::string &str
    ) {
        if( initiator == I_GROUP ) {
            str = group;
        } else if( initiator == I_CONSUMER || initiator == I_PRODUCER ) {
            str = group;
            str += " / ";
            str = channel;
            str += " / ";
            str += initiator == I_CONSUMER ? "consumer" : "producer";
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
        Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::string ceilType = "Fail";
        std::string ceilInitiator = "~";
        std::string ceilIP = "0.0.0.0";
        std::string ceilValues = "";

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
        Initiator initiator,
        const char *group,
        const char *channel,
        const char *login
    ) {
        std::string ceilType = "Success";
        std::string ceilInitiator = "~";
        std::string ceilIP = "0.0.0.0";
        std::string ceilValues = "";

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
 
