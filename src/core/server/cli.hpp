#ifndef SIMQ_CORE_SERVER_CLI
#define SIMQ_CORE_SERVER_CLI

#include "cli_callbacks.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

// TODO Красивый вывод

namespace simq::core::server {
    class CLI {
        private:
            CLICallbacks *_cb = nullptr;

            enum CtxNavigation {
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

            struct Navigation {
                CtxNavigation ctx;
                char *group;
                char *channel;
                char *login;
            };

            Navigation *_nav = nullptr;

            const char *_pathGroups = "groups";
            const char *_pathSettings = "settings";
            const char *_pathConsumers = "consumers";
            const char *_pathProducers = "producers";

            const char *_cmdLs = "ls";
            const char *_cmdH = "h";
            const char *_cmdCd = "cd";
            const char *_cmdInfo = "info";
            const char *_cmdSet = "set";
            const char *_cmdPswd = "passwd";
            const char *_cmdRemove = "rm";
            const char *_cmdAdd = "add";

            void _getCtxPath( std::string &path, CtxNavigation ctx );
            void _getLS( std::vector<std::string> &list );
            void _getAllowedCommands( std::vector<std::string> &list );

            Navigation *_parseNavigation( const char *line );
            void _goNext( Navigation *nav, const char *path );
            void _goBack( Navigation *nav );
            void _goRoot( Navigation *nav );
            void _freeNavigation( Navigation *nav );
            void _applyNavigation( Navigation *nav );

            void _parseCommand( const char *line );

        public:
            CLI( CLICallbacks *cb );
    };

    CLI::CLI( CLICallbacks *cb ) {
        _cb = cb;

        _nav = new Navigation{};
        _nav->ctx = CTX_GROUPS;

        while( true ) {
            std::string line;

            std::string path;
            _getCtxPath( path, _nav->ctx );

            std::cout << "simq ";
            std::cout << path;
            std::cout << "> ";
            std::getline( std::cin, line, '\n' );
            _parseCommand( line.c_str() );
        }

    }

    void CLI::_getCtxPath( std::string &path, CtxNavigation ctx ) {
        path = "/";
        switch( _nav->ctx ) {
            case CTX_ROOT:
                break;
            case CTX_GROUPS:
                path += _pathGroups;
                break;
            case CTX_GROUP:
                _getCtxPath( path, CTX_GROUPS );
                path += "/";
                path += _nav->group;
                break;
            case CTX_CHANNEL:
                _getCtxPath( path, CTX_GROUP );
                path += "/";
                path += _nav->channel;
                break;
            case CTX_CONSUMERS:
                _getCtxPath( path, CTX_CHANNEL );
                path += "/";
                path += _pathConsumers;
                break;
            case CTX_PRODUCERS:
                _getCtxPath( path, CTX_CHANNEL );
                path += "/";
                path += _pathProducers;
                break;
            case CTX_CONSUMER:
                _getCtxPath( path, CTX_CONSUMERS );
                path += "/";
                path += _nav->login;
                break;
            case CTX_PRODUCER:
                _getCtxPath( path, CTX_PRODUCERS );
                path += "/";
                path += _nav->login;
                break;
            case CTX_SETTINGS:
                path += _pathSettings;
                break;
        }
    }

    void CLI::_getLS( std::vector<std::string> &list ) {
        switch( _nav->ctx ) {
            case CTX_ROOT:
                list.push_back( _pathGroups );
                list.push_back( _pathSettings );
                break;
            case CTX_GROUPS:
                _cb->getGroups( list );
                break;
            case CTX_GROUP:
                _cb->getChannels( _nav->group, list );
                break;
            case CTX_CHANNEL:
                list.push_back( _pathConsumers );
                list.push_back( _pathProducers );
                break;
            case CTX_CONSUMERS:
                _cb->getConsumers( _nav->group, _nav->channel, list );
                break;
            case CTX_PRODUCERS:
                _cb->getProducers( _nav->group, _nav->channel, list );
                break;
        }
    }

    void CLI::_getAllowedCommands( std::vector<std::string> &list ) {
        list.push_back( _cmdH );
        list.push_back( _cmdCd );
        switch( _nav->ctx ) {
            case CTX_ROOT:
                list.push_back( _cmdLs );
                list.push_back( _cmdInfo );
                list.push_back( _cmdSet );
                list.push_back( _cmdPswd );
                break;
            case CTX_GROUPS:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdRemove );
                break;
            case CTX_GROUP:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdPswd );
                break;
            case CTX_CHANNEL:
                list.push_back( _cmdLs );
                list.push_back( _cmdInfo );
                list.push_back( _cmdSet );
                break;
            case CTX_CONSUMERS:
            case CTX_PRODUCERS:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdRemove );
                break;
            case CTX_PRODUCER:
            case CTX_CONSUMER:
                list.push_back( _cmdPswd );
                break;
            case CTX_SETTINGS:
                list.push_back( _cmdInfo );
                list.push_back( _cmdSet );
                list.push_back( _cmdPswd );
        }
    }

    CLI::Navigation *CLI::_parseNavigation( const char *line ) {
        // TODO дописать CD

        std::string path = "";
        bool isSlash = false;
        unsigned int point = 0;
        std::vector<std::string> list;

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            char ch = line[i];

            if( ch == '/' ) {
                if( i == 0 ) {
                    std::cout << "root" << std::endl;
                    // root
                } else if( path != "" ) {
                    std::cout << "next" << std::endl;
                    // next
                }

                path = "";
                point = 0;
                isSlash = true;
            } else if( ch == '.' ) {
                if( path != "" || point == 2 ) {
                    throw -1;
                }
                path = "";
                point++;
                isSlash = false;
                if( point == 2 ) {
                    std::cout << "back" << std::endl;
                    // back
                }
            } else if( ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' ) {
                if( point != 0 ) {
                    throw -1;
                }
                path += ch;
                isSlash = false;
            }
        }

        if( path != "" ) {
            std::cout << "next" << std::endl;
        }

        return nullptr;
    }

    void CLI::_parseCommand( const char *line ) {
        std::string cmd;
        std::string body;

        bool isCmd = false;
        bool isStart = false;
        bool toBody = false;

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            char ch = line[i];

            if( ch == ' ' ) {
                if( isStart ) {
                    toBody = true;
                }
            } else {
                isStart = true;
                if( !toBody ) {
                    cmd += ch;
                } else {
                    body += ch;
                }
            }
        }

        std::vector<std::string> allowedCommands;
        _getAllowedCommands( allowedCommands );

        auto it = std::find( allowedCommands.begin(), allowedCommands.end(), cmd );

        if( it != allowedCommands.end() ) {
            if( cmd == _cmdCd ) {
                _parseNavigation( body.c_str() );
            } else if( cmd == _cmdLs ) {
                std::vector<std::string> list;
                _getLS( list );
                std::cout << std::endl;
                for( auto it = list.begin(); it != list.end(); it++ ) {
                    std::cout << "    " << *it << std::endl;
                }
                std::cout << std::endl;
            } else if( cmd == _cmdH ) {
                std::vector<std::string> list;
                _getAllowedCommands( list );
                std::cout << std::endl;
                for( auto it = list.begin(); it != list.end(); it++ ) {
                    std::cout << "    " << *it << std::endl;
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << std::endl;
            std::cout << "    Unknown command. Use 'h' to get allowed commands" << std::endl;
            std::cout << std::endl;
        }
    }
}

#endif
