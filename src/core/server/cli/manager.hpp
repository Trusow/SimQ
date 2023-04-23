#ifndef SIMQ_CORE_SERVER_CLI_MANAGER
#define SIMQ_CORE_SERVER_CLI_MANAGER

#include "callbacks.hpp"
#include "navigation.hpp"
#include "scenario.h"
#include "scenario_auth.hpp"
#include "scenario_password.hpp"
#include "scenario_remove.hpp"
#include "scenario_add.hpp"
#include "ini.h"
#include "cmd_h.hpp"
#include "cmd_ls.hpp"
#include "cmd_info.hpp"
#include "cmd_set.hpp"
#include "cmd_cd.hpp"
#include "../../../util/string.hpp"
#include "../../../crypto/hash.hpp"
#include "../../../util/console_callbacks.h"
#include "../../../util/console.hpp"
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <map>

namespace simq::core::server::CLI {
    class Manager: public util::ConsoleCallbacks {
        private:
            Callbacks *_cb = nullptr;
            util::Console *_console = nullptr;
            CLI::Navigation *_nav = nullptr;

            std::string _scen = "";
            bool _isScenario = false;

            std::map<std::string, Scenario *> _listScenario;
            std::map<std::string, Cmd *> _listCmd;

            void _getAllowedCommands( std::vector<std::string> &list );
        public:
            Manager( Callbacks *cb );
            ~Manager();
            void input( std::vector<std::string> &list );
            void inputPassword( const char *password );
            void confirm( bool value );
    };

    Manager::Manager( Callbacks *cb ) {
        _console = new util::Console( this );
        _nav = new Navigation( _console, cb );
        _cb = cb;

        _listCmd[Ini::cmdLs] = new CmdLs( _console, _nav, cb );
        _listCmd[Ini::cmdH] = new CmdH( _console, _nav );
        _listCmd[Ini::cmdInfo] = new CmdInfo( _console, _nav, cb );
        _listCmd[Ini::cmdSet] = new CmdSet( _console, _nav, cb );
        _listCmd[Ini::cmdCd] = new CmdCd( _console, _nav, cb );

        _listScenario[Ini::cmdAuth] = new ScenarioAuth( _console, _nav, _cb );
        _listScenario[Ini::cmdPswd] = new ScenarioPassword( _console, _nav, _cb );
        _listScenario[Ini::cmdRemove] = new ScenarioRemove( _console, _nav, _cb );
        _listScenario[Ini::cmdAdd] = new ScenarioAdd( _console, _nav, _cb );

        _listScenario[Ini::cmdAuth]->start();
        _scen = Ini::cmdAuth;
        _isScenario = !_listScenario[_scen]->isEnd();

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
            list.push_back( Ini::cmdPswd );
        } else if( _nav->isSettings() ) {
            list.push_back( Ini::cmdInfo );
            list.push_back( Ini::cmdSet );
            list.push_back( Ini::cmdPswd );
        }
    }

    void Manager::input( std::vector<std::string> &list ) {
        if( !_isScenario ) {
            auto cmd = list[0];
            std::vector<std::string> allowedCommands;
            _getAllowedCommands( allowedCommands );

            auto it = std::find( allowedCommands.begin(), allowedCommands.end(), cmd );
            auto itCmd = _listCmd.find( cmd );
            auto itScneario = _listScenario.find( cmd );

            if( itScneario != _listScenario.end() && it != allowedCommands.end() ) {
                _scen = cmd;

                _listScenario[cmd]->start();
                list.erase( list.begin() );
                _listScenario[cmd]->input( list );

                _isScenario = !_listScenario[cmd]->isEnd();
            } else if( itCmd != _listCmd.end() && it != allowedCommands.end() ) {
                list.erase( list.begin() );
                _listCmd[cmd]->run( list );
            } else if( cmd[0] == '.' || cmd[0] == '/' ) {
                _listCmd[Ini::cmdCd]->run( list );
            } else {
                Ini::printDanger( _console, "Unknown command" );
            }
        } else {
            _listScenario[_scen]->input( list );
            _isScenario = !_listScenario[_scen]->isEnd();
        }
    }

    void Manager::inputPassword( const char *password ) {
        _listScenario[_scen]->password( password );
        _isScenario = !_listScenario[_scen]->isEnd();
    }

    void Manager::confirm( bool value ) {
        _listScenario[_scen]->confirm( value );
        _isScenario = !_listScenario[_scen]->isEnd();
    }
}

#endif
