#ifndef SIMQ_CORE_SERVER_CLI_CMD_CD
#define SIMQ_CORE_SERVER_CLI_CMD_CD

#include "callbacks.hpp"
#include "../../../util/console.hpp"
#include "ini.h"
#include "cmd.h"
#include "navigation.hpp"
#include <string>
#include <vector>
#include <algorithm>

namespace simq::core::server::CLI {
    class CmdCd: public Cmd {
        private:
            Callbacks *_cb = nullptr;
            util::Console *_console = nullptr;
            Navigation *_nav = nullptr;

            Navigation::Ctx _ctx = Navigation::CTX_GROUPS;
            std::string _group = "";
            std::string _channel = "";
            std::string _login = "";

            bool _isAllowedPath(
                Navigation::Ctx ctx,
                std::string &group,
                std::string &channel,
                std::string &login
            );
            void _build( const char *path );

            void _toRoot( Navigation::Ctx &ctx );
            void _toNext(
                Navigation::Ctx &ctx,
                std::string &group,
                std::string &channel,
                std::string &login,
                std::string &path
            );
            void _toBack(
                Navigation::Ctx &ctx,
                std::string &group,
                std::string &channel,
                std::string &login
            );
        public:
            CmdCd( util::Console *console, Navigation *nav, Callbacks *cb ) : _console{console}, _nav{nav}, _cb{cb} {}

            void run( std::vector<std::string> &params );
    };

    bool CmdCd::_isAllowedPath(
        Navigation::Ctx ctx,
        std::string &group,
        std::string &channel,
        std::string &login
    ) {
        std::vector<std::string> list;
        bool res;

        switch( ctx ) {
            case Navigation::CTX_GROUP:
                _cb->getGroups( list );
                return std::find( list.begin(), list.end(), group ) != list.end();
            case Navigation::CTX_CHANNEL:
            case Navigation::CTX_CONSUMERS:
            case Navigation::CTX_PRODUCERS:
                _cb->getChannels( group.c_str(), list );
                return std::find( list.begin(), list.end(), channel ) != list.end();
                break;
            case Navigation::CTX_CONSUMER:
                _cb->getConsumers( group.c_str(), channel.c_str(), list );
                return std::find( list.begin(), list.end(), login ) != list.end();
                break;
            case Navigation::CTX_PRODUCER:
                _cb->getProducers( group.c_str(), channel.c_str(), list );
                return std::find( list.begin(), list.end(), login ) != list.end();
                break;
        }

        return false;
    }

    void CmdCd::_toRoot( Navigation::Ctx &ctx ) {
        ctx = Navigation::CTX_ROOT;
    }

    void CmdCd::_toBack(
        Navigation::Ctx &ctx,
        std::string &group,
        std::string &channel,
        std::string &login
    ) {
        bool check = true;

        switch( ctx ) {
            case Navigation::CTX_ROOT:
            case Navigation::CTX_GROUPS:
            case Navigation::CTX_SETTINGS:
                ctx = Navigation::CTX_ROOT;
                check = false;
                break;
            case Navigation::CTX_GROUP:
                ctx = Navigation::CTX_GROUPS;
                check = false;
                break;
            case Navigation::CTX_CHANNEL:
                ctx = Navigation::CTX_GROUP;
                break;
            case Navigation::CTX_CONSUMERS:
            case Navigation::CTX_PRODUCERS:
                ctx = Navigation::CTX_CHANNEL;
                break;
            case Navigation::CTX_CONSUMER:
                ctx = Navigation::CTX_CONSUMERS;
                break;
            case Navigation::CTX_PRODUCER:
                ctx = Navigation::CTX_PRODUCERS;
                break;
        }

        if( check ) {
            if( !_isAllowedPath( ctx, group, channel, login ) ) {
                throw -1;
            }
        }

    }

    void CmdCd::_toNext(
        Navigation::Ctx &ctx,
        std::string &group,
        std::string &channel,
        std::string &login,
        std::string &path
    ) {
        switch( ctx ) {
            case Navigation::CTX_ROOT:
                if( path == Ini::pathGroups ) {
                    ctx = Navigation::CTX_GROUPS;
                } else if( path == Ini::pathSettings ) {
                    ctx = Navigation::CTX_SETTINGS;
                } else {
                    throw -1;
                }
                break;
            case Navigation::CTX_GROUPS:
                ctx = Navigation::CTX_GROUP;
                group = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case Navigation::CTX_GROUP:
                ctx = Navigation::CTX_CHANNEL;
                channel = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case Navigation::CTX_CHANNEL:
                if( path == Ini::pathConsumers ) {
                    ctx = Navigation::CTX_CONSUMERS;
                } else if( path == Ini::pathProducers ) {
                    ctx = Navigation::CTX_PRODUCERS;
                } else {
                    throw -1;
                }
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case Navigation::CTX_CONSUMERS:
                ctx = Navigation::CTX_CONSUMER;
                login = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            case Navigation::CTX_PRODUCERS:
                ctx = Navigation::CTX_PRODUCER;
                login = path;
                if( !_isAllowedPath( ctx, group, channel, login ) ) {
                    throw -1;
                }
                break;
            default:
                throw -1;
        }
    }

    void CmdCd::_build( const char *path ) {
        bool isSlash = false;
        unsigned int point = 0;
        std::vector<std::string> list;
        std::string part = "";

        Navigation::Ctx localCtx = _ctx;
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
        _nav->apply( _ctx, _group.c_str(), _channel.c_str(), _login.c_str() );
    }

    void CmdCd::run( std::vector<std::string> &params ) {
        if( params.empty() ) {
            _console->setPrefix( _nav->getPathWithPrefix() );
            _console->printPrefix( true );
            return;
        } else if ( params.size() > 1 ) {
            _console->printDanger( "Many params" );
            _console->printPrefix();
            return;
        }

        try {
            _ctx = _nav->getCtx();
            _group = _nav->getGroup();
            _channel = _nav->getChannel();
            _login = _nav->getUser();

            _build( params[0].c_str() );
            _console->setPrefix( _nav->getPathWithPrefix() );
            _console->printPrefix( true );
        } catch( ... ) {
            _console->printDanger( "Not found path" );
            _console->printPrefix();
        }
    }
}

#endif
