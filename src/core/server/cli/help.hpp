#ifndef SIMQ_CORE_SERVER_CLI_HELP
#define SIMQ_CORE_SERVER_CLI_HELP

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "ini.h"
#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class Help {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
        public:
            Help(
                util::Console *console,
                Navigation *nav
            ) : _console{console}, _nav{nav} {};
            void print( std::vector<std::string> &list );
    };

    void Help::print( std::vector<std::string> &list ) {
        std::vector<std::string> help;

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
                    str += " - show settings of server";
                } else if( _nav->isChannel() ) {
                    str += " - show settings of channel";
                }
            } else if( cmd == Ini::cmdSet ) {
                if( _nav->isSettings() ) {
                    str += " - save settings of server, example '";
                    str += cmd + " nameField valueField'";
                } else if( _nav->isChannel() ) {
                    str += " - save settings of server, example '";
                    str += cmd + " nameField valueField'";
                }
            } else if( cmd == Ini::cmdLs ) {
                str += " - show available paths, is set filter example '";
                str += cmd + " query'";
            } else if( cmd == Ini::cmdCd ) {
                str += " - go to path";
            }

            help.push_back( str );

        }

        _console->printList( help );
        _console->printPrefix();
    }

}

#endif
