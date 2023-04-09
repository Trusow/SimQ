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

            Ctx _ctx = CTX_GROUPS;

            struct CDCommand {
                Ctx ctx;
                char *group;
                char *channel;
                char *login;
            };

            char *_group = nullptr;
            char *_channel = nullptr;
            char *_login = nullptr;

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

            void _getCtxPath( std::string &path );
            void _getLS( std::vector<std::string> &list );
            void _getAllowedCommands( std::vector<std::string> &list );
            CDCommand *_parseCD( const char *line );
            void _freeCD( CDCommand *command );
            void _parseCommand( const char *line );

        public:
            CLI( CLICallbacks *cb );
    };

    CLI::CLI( CLICallbacks *cb ) {
        _cb = cb;

        while( true ) {
            std::string line;

            std::string path;
            _getCtxPath( path );

            std::cout << "simq ";
            std::cout << path;
            std::cout << "> ";
            std::getline( std::cin, line, '\n' );
            _parseCommand( line.c_str() );
        }

    }

    void CLI::_getCtxPath( std::string &path ) {
        path = "/";
        switch( _ctx ) {
            case CTX_ROOT:
                break;
            case CTX_GROUPS:
                path += _pathGroups;
                break;
            case CTX_GROUP:
                path += _pathGroups;
                path += "/";
                path += _group;
                break;
            case CTX_CHANNEL:
                path += _pathGroups;
                path += "/";
                path += _group;
                path += "/";
                path += _channel;
                break;
            case CTX_CONSUMERS:
                path += _pathGroups;
                path += "/";
                path += _group;
                path += "/";
                path += _channel;
                path += "/";
                path += _pathConsumers;
                break;
            case CTX_PRODUCERS:
                path += _pathGroups;
                path += "/";
                path += _group;
                path += "/";
                path += _channel;
                path += "/";
                path += _pathProducers;
                break;
            case CTX_CONSUMER:
                path += _pathGroups;
                path += "/";
                path += _group;
                path += "/";
                path += _channel;
                path += "/";
                path += _pathConsumers;
                path += "/";
                path += _login;
                break;
            case CTX_PRODUCER:
                path += _pathGroups;
                path += "/";
                path += _group;
                path += "/";
                path += _channel;
                path += "/";
                path += _pathProducers;
                path += "/";
                path += _login;
                break;
            case CTX_SETTINGS:
                path += _pathSettings;
                break;
        }
    }

    void CLI::_getLS( std::vector<std::string> &list ) {
        switch( _ctx ) {
            case CTX_ROOT:
                list.push_back( _pathGroups );
                list.push_back( _pathSettings );
                break;
            case CTX_GROUPS:
                _cb->getGroups( list );
                break;
            case CTX_GROUP:
                _cb->getChannels( _group, list );
                break;
            case CTX_CHANNEL:
                list.push_back( _pathConsumers );
                list.push_back( _pathProducers );
                break;
            case CTX_CONSUMERS:
                _cb->getConsumers( _group, _channel, list );
                break;
            case CTX_PRODUCERS:
                _cb->getProducers( _group, _channel, list );
                break;
        }
    }

    void CLI::_getAllowedCommands( std::vector<std::string> &list ) {
        list.push_back( _cmdH );
        list.push_back( _cmdCd );
        switch( _ctx ) {
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

    CLI::CDCommand *CLI::_parseCD( const char *line ) {
        // TODO дописать CD

        std::string path = "";
        bool isSlash = false;
        unsigned int point = 0;
        std::vector<std::string> list;

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            char ch = line[i];

            if( ch == '/' ) {
                if( isSlash || point == 1 ) {
                    isSlash = false;
                    continue;
                } else if( path != "" ) {
                    list.clear();
                    _getLS( list );
                    auto it = std::find( list.begin(), list.end(), path );
                    if( it != list.end() ) {
                    } else {
                        std::cout << "wrong param" << std::endl;
                    }
                } else if( point == 2 ) {
                }
                path = "";
                point = 0;
                isSlash = true;
            } else if( ch == '.' ) {
                if( point == 2 ) {
                    throw -1;
                }
                path = "";
                point++;
                isSlash = false;
            } else if( ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' ) {
                if( point != 0 ) {
                    throw -1;
                }
                path += ch;
                isSlash = false;
            }
        }

        if( isSlash ) {
            _ctx = CTX_ROOT;
        } if( path != "" ) {
            list.clear();
            _getLS( list );
            auto it = std::find( list.begin(), list.end(), path );
            if( it != list.end() ) {
                switch( _ctx ) {
                    case CTX_ROOT:
                        if( path == _pathGroups ) {
                            _ctx = CTX_GROUPS;
                        } else {
                            _ctx = CTX_SETTINGS;
                        }
                        break;
                }
            } else {
                std::cout << "wrong param" << std::endl;
            }
        } else if( point == 2 ) {
            switch( _ctx ) {
                case CTX_GROUPS:
                case CTX_SETTINGS:
                    _ctx = CTX_ROOT;
                    break;
            }
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
                _parseCD( body.c_str() );
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
