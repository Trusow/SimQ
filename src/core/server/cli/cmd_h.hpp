#ifndef SIMQ_CORE_SERVER_CLI_CMD_H
#define SIMQ_CORE_SERVER_CLI_CMD_H

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "ini.h"
#include "cmd.h"
#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class CmdH: public Cmd {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
        public:
            CmdH(
                util::Console *console,
                Navigation *nav
            ) : _console{console}, _nav{nav} {};
            void run( std::vector<std::string> &params );
    };

    void CmdH::run( std::vector<std::string> &params ) {
        if( params.size() > 1 ) {
            Ini::printDanger( _console, "Many params" );
            return;
        }

        std::vector<std::string> help;

        std::vector<std::string> list;
        list.push_back( Ini::cmdLs );
        list.push_back( Ini::cmdCd );
        list.push_back( Ini::cmdAdd );
        list.push_back( Ini::cmdRemove );
        list.push_back( Ini::cmdInfo );
        list.push_back( Ini::cmdSet );
        list.push_back( Ini::cmdPswd );

        for( unsigned int i = 0; i < list.size(); i++ ) {
            std::string cmd = list[i];
            if( cmd == Ini::cmdH ) {
                continue;
            }

            std::string str = cmd;

            if( cmd == Ini::cmdAdd ) {
                if( _nav->isGroups() ) {
                    str += " - create group, example '";
                    str += cmd + " groupTest'";
                } else if( _nav->isGroup() ) {
                    str += " - create channel, example '";
                    str += cmd + " channelTest'";
                } else if( _nav->isConsumers() ) {
                    str += " - create consumer, example '";
                    str += cmd + " consumerTest'";
                } else if( _nav->isProducers() ) {
                    str += " - create producer, example '";
                    str += cmd + " producerTest'";
                }
            } else if( cmd == Ini::cmdRemove ) {
                if( _nav->isGroups() ) {
                    str += " - remove group, example '";
                    str += cmd + " groupTest'";
                } else if( _nav->isGroup() ) {
                    str += " - remove channel, example '";
                    str += cmd + " channelTest'";
                } else if( _nav->isConsumers() ) {
                    str += " - remove consumer, example '";
                    str += cmd + " consumerTest'";
                } else if( _nav->isProducers() ) {
                    str += " - remove producer, example '";
                    str += cmd + " producerTest'";
                }
            } else if( cmd == Ini::cmdPswd ) {
                if( _nav->isGroup() ) {
                    str += " - update password of group";
                } else if( _nav->isConsumer() ) {
                    str += " - update password of consumer";
                } else if( _nav->isProducer() ) {
                    str += " - update password of producer";
                } else if( _nav->isSettings() ) {
                    str += " - update password of server";
                }
            } else if( cmd == Ini::cmdInfo ) {
                if( _nav->isSettings() ) {
                    str += " - show settings of server, is set filter example '";
                    str += cmd + " query'";
                } else if( _nav->isChannel() ) {
                    str += " - show settings of channel, is set filter example '";
                    str += cmd + " query'";
                }
            } else if( cmd == Ini::cmdSet ) {
                if( _nav->isSettings() ) {
                    str += " - save settings of server, example '";
                    str += cmd + " ";
                    str += Ini::infoSettingsCountThreads;
                    str += " 4'";
                } else if( _nav->isChannel() ) {
                    str += " - save settings of server, example '";
                    str += cmd + " ";
                    str += Ini::infoChMinMessageSize;
                    str += " 100'";
                }
            } else if( cmd == Ini::cmdLs ) {
                if( !_nav->isSettings() && !_nav->isConsumer() && !_nav->isProducer() ) {
                    str += " - show available paths, is set filter example '";
                    str += cmd + " query'";
                }
            } else if( cmd == Ini::cmdCd ) {
                str += " - go to path";
            }

            if( str != cmd ) {
                help.push_back( str );
            }

        }

        _console->printList( help, params.empty() ? nullptr : params[0].c_str() );
        _console->printPrefix();
    }

}

#endif
