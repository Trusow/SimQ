#ifndef SIMQ_CORE_SERVER_CLI_NAVIGATION
#define SIMQ_CORE_SERVER_CLI_NAVIGATION

#include "callbacks.hpp"
#include "../../../util/console.hpp"
#include "ini.h"
#include <string>
#include <vector>
#include <algorithm>

namespace simq::core::server::CLI {
    class Navigation {
        public:
            enum Ctx {
                CTX_ROOT,

                CTX_GROUPS,
                CTX_GROUP,
                CTX_CHANNEL,
                CTX_CONSUMERS,
                CTX_PRODUCERS,
                CTX_CONSUMER,
                CTX_PRODUCER,

                CTX_SETTINGS,
            };
        private:

            Callbacks *_cb = nullptr;
            util::Console *_console = nullptr;

            std::string _path = "";
            std::string _pathWithPrefix = "";

            Ctx _ctx = CTX_GROUPS;
            std::string _group = "";
            std::string _channel = "";
            std::string _login = "";

            void _buildPath( Ctx ctx );

        public:
            Navigation( util::Console *console, Callbacks *cb ) : _console{console}, _cb{cb} {}

            const char *getPath();
            const char *getPathWithPrefix();

            void apply( Ctx ctx, const char *group, const char *channel, const char *login );

            bool isRoot();
            bool isSettings();
            bool isGroups();
            bool isGroup();
            bool isChannel();
            bool isConsumers();
            bool isConsumer();
            bool isProducers();
            bool isProducer();

            Ctx getCtx();
            const char *getGroup();
            const char *getChannel();
            const char *getUser();
    };

    void Navigation::_buildPath( Ctx ctx ) {
        switch( ctx ) {
            case CTX_ROOT:
                break;
            case CTX_GROUPS:
                _path += Ini::pathGroups;
                break;
            case CTX_GROUP:
                _buildPath( CTX_GROUPS );
                _path += "/";
                _path += _group;
                break;
            case CTX_CHANNEL:
                _buildPath( CTX_GROUP );
                _path += "/";
                _path += _channel;
                break;
            case CTX_CONSUMERS:
                _buildPath( CTX_CHANNEL );
                _path += "/";
                _path += Ini::pathConsumers;
                break;
            case CTX_PRODUCERS:
                _buildPath( CTX_CHANNEL );
                _path += "/";
                _path += Ini::pathProducers;
                break;
            case CTX_CONSUMER:
                _buildPath( CTX_CONSUMERS );
                _path += "/";
                _path += _login;
                break;
            case CTX_PRODUCER:
                _buildPath( CTX_PRODUCERS );
                _path += "/";
                _path += _login;
                break;
            case CTX_SETTINGS:
                _path += Ini::pathSettings;
                break;
        }
    }

    const char *Navigation::getPath() {
        _path = "/";
        _buildPath( _ctx );
        return _path.c_str();
    }

    const char *Navigation::getPathWithPrefix() {
        _path = "/";
        _buildPath( _ctx );

        _pathWithPrefix = Ini::defPrefix;
        _pathWithPrefix += ": ";
        _pathWithPrefix += _path;
        _pathWithPrefix += "> ";

        return _pathWithPrefix.c_str();
    }
    
    const char *Navigation::getGroup() {
        return _group.c_str();
    }

    const char *Navigation::getChannel() {
        return _channel.c_str();
    }

    const char *Navigation::getUser() {
        return _login.c_str();
    }

    bool Navigation::isRoot() {
        return _ctx == CTX_ROOT;
    }

    bool Navigation::isGroups() {
        return _ctx == CTX_GROUPS;
    }

    bool Navigation::isGroup() {
        return _ctx == CTX_GROUP;
    }

    bool Navigation::isSettings() {
        return _ctx == CTX_SETTINGS;
    }

    bool Navigation::isChannel() {
        return _ctx == CTX_CHANNEL;
    }

    bool Navigation::isConsumers() {
        return _ctx == CTX_CONSUMERS;
    }

    bool Navigation::isConsumer() {
        return _ctx == CTX_CONSUMER;
    }

    bool Navigation::isProducers() {
        return _ctx == CTX_PRODUCERS;
    }

    bool Navigation::isProducer() {
        return _ctx == CTX_PRODUCER;
    }

    void Navigation::apply(
        Ctx ctx,
        const char *group,
        const char *channel,
        const char *login
    ) {
        _ctx = ctx;
        _group = group;
        _channel = channel;
        _login = login;
    }

    Navigation::Ctx Navigation::getCtx() {
        return _ctx;
    }
}

#endif
