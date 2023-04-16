#ifndef SIMQ_CORE_SERVER_CLI_MANAGER
#define SIMQ_CORE_SERVER_CLI_MANAGER

#include "callbacks.hpp"
#include "navigation.hpp"
#include "scenario.h"
#include "scenario_auth.hpp"
#include "ini.h"
#include "help.hpp"
#include "ls.hpp"
#include "../../../util/string.hpp"
#include "../../../crypto/hash.hpp"
#include "../../../util/console_callbacks.h"
#include "../../../util/console.hpp"
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <iostream>

namespace simq::core::server::CLI {
    class Manager: public util::ConsoleCallbacks {
        private:
            Callbacks *_cb = nullptr;
            util::Console *_console = nullptr;
            CLI::Navigation *_nav = nullptr;

            enum Scen {
                SCENARIO_NONE,
                SCENARIO_AUTH,
                SCENARIO_ADD,
                SCENARIO_REMOVE,
                SCENARIO_UPSWD,
            };

            Scen _scen = SCENARIO_NONE;

            Scenario *_scenAuth = nullptr;
            Scenario *_scenAdd = nullptr;
            Scenario *_scenRemove = nullptr;
            Scenario *_scenUpswd = nullptr;
            Help *_help = nullptr;
            Ls *_ls = nullptr;

            bool _isAuth = false;

            const char *_warningChanges = "The changes will be applied by the server.";
            const char *_errNotFoundPath = "Not found path.";

            void _getAllowedCommands( std::vector<std::string> &list );

            void _cd( std::vector<std::string> &list );

            void _auth( const char *password );

        public:
            Manager( Callbacks *cb );
            ~Manager();
            void input( std::vector<std::string> &list );
            void inputPassword( const char *password );
            void confirm( bool value );
    };

    Manager::Manager( Callbacks *cb ) {
        _nav = new Navigation( cb );
        _cb = cb;


        _console = new util::Console( this );
        _help = new Help( _console, _nav );
        _ls = new Ls( _console, _nav, cb );

        _scenAuth = new ScenarioAuth( _console, _nav, _cb );
        _scenAuth->start();
        _scen = SCENARIO_AUTH;
       
        _console->run();
    }

    Manager::~Manager() {
        delete this->_console;
    }

    void Manager::_getAllowedCommands( std::vector<std::string> &list ) {

        list.push_back( Ini::cmdH );
        list.push_back( Ini::cmdCd );

        if( _nav->isRoot() ) {
            list.push_back( Ini::cmdLs );
        } else if( _nav->isGroups() ) {
            list.push_back( Ini::cmdLs );
            list.push_back( Ini::cmdAdd );
            list.push_back( Ini::cmdRemove );
        } else if( _nav->isGroup() ) {
            list.push_back( Ini::cmdLs );
            list.push_back( Ini::cmdAdd );
            list.push_back( Ini::cmdRemove );
            list.push_back( Ini::cmdPswd );
        } else if( _nav->isChannel() ) {
            list.push_back( Ini::cmdLs );
            list.push_back( Ini::cmdInfo );
            list.push_back( Ini::cmdSet );
        } else if( _nav->isConsumers() || _nav->isProducers() ) {
            list.push_back( Ini::cmdLs );
            list.push_back( Ini::cmdAdd );
            list.push_back( Ini::cmdRemove );
        } else if( _nav->isConsumer() || _nav->isProducer() ) {
            list.push_back( Ini::cmdInfo );
        } else if( _nav->isSettings() ) {
            list.push_back( Ini::cmdInfo );
            list.push_back( Ini::cmdSet );
            list.push_back( Ini::cmdPswd );
        }
    }

    void Manager::_cd( std::vector<std::string> &list ) {
        if( list.size() != 2 ) {
            _console->printDanger( "Wrong param" );
            _console->printPrefix();
            return;
        }

        try {
            _nav->to( list[1].c_str() );
            std::string path = Ini::defPrefix;
            path += ": ";
            path += _nav->getPath();
            path += "> ";
            _console->setPrefix( path.c_str() );
            _console->printPrefix( true );
        } catch( ... ) {
            _console->printDanger( _errNotFoundPath );
            _console->printPrefix();
        }
    }


    void Manager::input( std::vector<std::string> &list ) {
        if( _scen == SCENARIO_NONE ) {
            auto cmd = list[0];

            std::vector<std::string> allowedCommands;
            _getAllowedCommands( allowedCommands );

            auto it = std::find( allowedCommands.begin(), allowedCommands.end(), cmd );

            if( it == allowedCommands.end() ) {
                _console->printDanger( "Unknown command" );
                _console->printPrefix();
            } else if( cmd == Ini::cmdLs ) {
                if( list.size() > 1 ) {
                    _ls->print( list[1].c_str() );
                } else {
                    _ls->print();
                }
            } else if( cmd == Ini::cmdCd ) {
                _cd( list );
            } else if( cmd == Ini::cmdH ) {
                _help->print( allowedCommands );
            } else if( cmd == Ini::cmdRemove ) {
            } else if( cmd == Ini::cmdAdd ) {
            }
        } else {
            switch( _scen ) {
                case SCENARIO_AUTH:
                    _scenAuth->input( list );
                    if( _scenAuth->isEnd() ) {
                        _scen = SCENARIO_NONE;
                    }
                    break;
                case SCENARIO_ADD:
                    _scenAdd->input( list );
                    if( _scenAdd->isEnd() ) {
                        _scen = SCENARIO_NONE;
                    }
                    break;
                case SCENARIO_REMOVE:
                    _scenRemove->input( list );
                    if( _scenRemove->isEnd() ) {
                        _scen = SCENARIO_NONE;
                    }
                    break;
                case SCENARIO_UPSWD:
                    _scenUpswd->input( list );
                    if( _scenUpswd->isEnd() ) {
                        _scen = SCENARIO_NONE;
                    }
                    break;
            }
        }
    }

    void Manager::inputPassword( const char *password ) {
        switch( _scen ) {
            case SCENARIO_AUTH:
                _scenAuth->password( password );
                if( _scenAuth->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_ADD:
                _scenAdd->password( password );
                if( _scenAdd->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_REMOVE:
                _scenRemove->password( password );
                if( _scenRemove->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_UPSWD:
                _scenUpswd->password( password );
                if( _scenUpswd->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
        }
    }

    void Manager::confirm( bool value ) {
        switch( _scen ) {
            case SCENARIO_AUTH:
                _scenAuth->confirm( value );
                if( _scenAuth->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_ADD:
                _scenAdd->confirm( value );
                if( _scenAdd->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_REMOVE:
                _scenRemove->confirm( value );
                if( _scenRemove->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
            case SCENARIO_UPSWD:
                _scenUpswd->confirm( value );
                if( _scenUpswd->isEnd() ) {
                    _scen = SCENARIO_NONE;
                }
                break;
        }
    }
}

#endif
