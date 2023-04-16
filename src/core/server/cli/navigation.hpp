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
        private:
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

            Callbacks *_cb = nullptr;
            util::Console *_console = nullptr;

            std::string _path = "";
            std::string _pathWithPrefix = "";

            Ctx _ctx = CTX_GROUPS;
            std::string _group = "";
            std::string _channel = "";
            std::string _login = "";

            bool _isAllowedPath(
                Ctx ctx,
                std::string &group,
                std::string &channel,
                std::string &login
            );
            void _build( const char *path );
            void _buildPath( Ctx ctx );

            void _toRoot( Ctx &ctx );
            void _toNext(
                Ctx &ctx,
                std::string &group,
                std::string &channel,
                std::string &login,
                std::string &path
            );
            void _toBack(
                Ctx &ctx,
                std::string &group,
                std::string &channel,
                std::string &login
            );
        public:
            Navigation( util::Console *console, Callbacks *cb ) : _console{console}, _cb{cb} {}

            void to( const char *path );
            const char *getPath();
            const char *getPathWithPrefix();

            bool isRoot();
            bool isSettings();
            bool isGroups();
            bool isGroup();
            bool isChannel();
            bool isConsumers();
            bool isConsumer();
            bool isProducers();
            bool isProducer();

            const char *getGroup();
            const char *getChannel();
            const char *getUser();
    };

    bool Navigation::_isAllowedPath(
        Ctx ctx,
        std::string &group,
        std::string &channel,
        std::string &login
    ) {
        std::vector<std::string> list;
        bool res;

        switch( ctx ) {
            case CTX_GROUP:
                _cb->getGroups( list );
                return std::find( list.begin(), list.end(), group ) != list.end();
            case CTX_CHANNEL:
            case CTX_CONSUMERS:
            case CTX_PRODUCERS:
                _cb->getChannels( group.c_str(), list );
                return std::find( list.begin(), list.end(), channel ) != list.end();
                break;
            case CTX_CONSUMER:
                _cb->getConsumers( group.c_str(), channel.c_str(), list );
                return std::find( list.begin(), list.end(), login ) != list.end();
                break;
            case CTX_PRODUCER:
                _cb->getProducers( group.c_str(), channel.c_str(), list );
                return std::find( list.begin(), list.end(), login ) != list.end();
                break;
        }

        return false;
    }

    void Navigation::_toRoot( Ctx &ctx ) {
        ctx = CTX_ROOT;
    }

    void Navigation::_toBack(
        Ctx &ctx,
        std::string &group,
        std::string &channel,
        std::string &login
    ) {
        bool check = true;

        switch( ctx ) {
            case CTX_ROOT:
            case CTX_GROUPS:
            case CTX_SETTINGS:
                ctx = CTX_ROOT;
                check = false;
                break;
            case CTX_GROUP:
                ctx = CTX_GROUPS;
                check = false;
                break;
            case CTX_CHANNEL:
                ctx = CTX_GROUP;
                break;
            case CTX_CONSUMERS:
            case CTX_PRODUCERS:
                ctx = CTX_CHANNEL;
                break;
            case CTX_CONSUMER:
                ctx = CTX_CONSUMERS;
                break;
            case CTX_PRODUCER:
                ctx = CTX_PRODUCERS;
                break;
        }

        if( check ) {
            if( !_isAllowedPath( ctx, group, channel, login ) ) {
                throw -1;
            }
        }

    }

    void Navigation::_toNext(
        Ctx &ctx,
        std::string &group,
        std::string &channel,
        std::string &login,
        std::string &path
    ) {
        switch( ctx ) {
            case CTX_ROOT:
                if( path == Ini::pathGroups ) {
                    ctx = CTX_GROUPS;
                } else if( path == Ini::pathSettings ) {
                    ctx = CTX_SETTINGS;
                } else {
                    throw -1;
                }
                break;
            case CTX_GROUPS:
                ctx = CTX_GROUP;
                group = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case CTX_GROUP:
                ctx = CTX_CHANNEL;
                channel = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case CTX_CHANNEL:
                if( path == Ini::pathConsumers ) {
                    ctx = CTX_CONSUMERS;
                } else if( path == Ini::pathProducers ) {
                    ctx = CTX_PRODUCERS;
                } else {
                    throw -1;
                }
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case CTX_CONSUMERS:
                ctx = CTX_CONSUMER;
                login = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case CTX_PRODUCERS:
                ctx = CTX_PRODUCER;
                login = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            default:
                throw -1;
        }
    }

    void Navigation::_build( const char *path ) {
        bool isSlash = false;
        unsigned int point = 0;
        std::vector<std::string> list;
        std::string part = "";

        Ctx localCtx = _ctx;
        std::string localGroup = _group;
        std::string localChannel = _channel;
        std::string localLogin = _login;

        for( unsigned int i = 0; path[i] != 0; i++ ) {
            char ch = path[i];

            if( ch == '/' ) {
                if( i == 0 ) {
                    _toRoot( localCtx );
                } else if( part != "" ) {
                    _toNext( localCtx, localGroup, localChannel, localLogin, part );
                }

                part = "";
                point = 0;
                isSlash = true;
            } else if( ch == '.' ) {
                if( part != "" || point == 2 ) {
                    throw -1;
                }
                part = "";
                point++;
                isSlash = false;
                if( point == 2 ) {
                    _toBack( localCtx, localGroup, localChannel, localLogin );
                }
            } else {
                if( point != 0 ) {
                    throw -1;
                }
                part += ch;
                isSlash = false;
            }
        }

        if( part != "" ) {
            _toNext( localCtx, localGroup, localChannel, localLogin, part );
        }

        _ctx = localCtx;
        _group = localGroup;
        _channel = localChannel;
        _login = localLogin;
    }

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

    void Navigation::to( const char *path ) {
        try {
            _build( path );
            _console->setPrefix( getPathWithPrefix() );
            _console->printPrefix( true );
        } catch( ... ) {
            _console->printDanger( "Not found path" );
            _console->printPrefix();
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
}

#endif
