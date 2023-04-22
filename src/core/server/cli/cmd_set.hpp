#ifndef SIMQ_CORE_SERVER_CLI_CMD_SET
#define SIMQ_CORE_SERVER_CLI_CMD_SET

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "../../../util/validation.hpp"
#include "../../../util/types.h"
#include "callbacks.hpp"
#include "ini.h"
#include "cmd.h"
#include <vector>
#include <string>
#include <stdlib.h>

namespace simq::core::server::CLI {
    class CmdSet: public Cmd {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            Callbacks *_cb = nullptr;
        public:
            CmdSet(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};
            void run( std::vector<std::string> &params );
    };

    void CmdSet::run( std::vector<std::string> &params ) {
        bool isErr = false;

        if( params.empty() ) {
            _console->printDanger( "Not found name of param" );
            isErr = true;
        } else if( params.size() == 1 ) {
            _console->printDanger( "Not found value of param" );
            isErr = true;
        } else if( params.size() > 2 ) {
            _console->printDanger( "Many params" );
            isErr = true;
        }

        if( isErr ) {
            _console->printPrefix();
            return;
        }

        unsigned int num = 0;
        bool isChannel = false;
        util::Types::ChannelSettings settings;

        auto name = params[0];
        auto value = params[1].c_str();

        if( value[0] != 0 && !util::Validation::isUInt( value ) ) {
            _console->printDanger( "Wrong value" );
            _console->printPrefix();
            return;
        } else {
            num = atol( value );
        }

        try {
            if( name == Ini::infoSettingsPort ) {
                if( !util::Validation::isPort( num ) ) {
                    _console->printDanger( "Wrong value" );
                    _console->printPrefix();
                } else {
                    _cb->updatePort( num );
                    _console->printSuccess( "Restart server to apply changes" );
                    _console->printPrefix();
                }
            } else if( name == Ini::infoSettingsCountThreads ) {
                if( !util::Validation::isCountThread( num ) ) {
                    _console->printDanger( "Wrong value" );
                    _console->printPrefix();
                } else {
                    _cb->updateCountThreads( num );
                    _console->printSuccess( "Restart server to apply changes" );
                    _console->printPrefix();
                }
            } else if( name == Ini::infoChMinMessageSize ) {
                _cb->getChannelSettings(
                    _nav->getGroup(),
                    _nav->getChannel(),
                    settings
                );
                settings.minMessageSize = num;
                isChannel = true;
            } else if( name == Ini::infoChMaxMessageSize ) {
                _cb->getChannelSettings(
                    _nav->getGroup(),
                    _nav->getChannel(),
                    settings
                );
                settings.maxMessageSize = num;
                isChannel = true;
            } else if( name == Ini::infoChMaxMessagesInMemory ) {
                _cb->getChannelSettings(
                    _nav->getGroup(),
                    _nav->getChannel(),
                    settings
                );
                settings.maxMessagesInMemory = num;
                isChannel = true;
            } else if( name == Ini::infoChMaxMessagesOnDisk ) {
                _cb->getChannelSettings(
                    _nav->getGroup(),
                    _nav->getChannel(),
                    settings
                );
                settings.maxMessagesOnDisk = num;
                isChannel = true;
            } else {
                _console->printDanger( "Unknown name" );
                _console->printPrefix();
            }

            if( isChannel ) {
                if( !util::Validation::isChannelSettings( &settings ) ) {
                    _console->printDanger( "Wrong value" );
                    _console->printPrefix();
                } else {
                    _console->printWarning( "The changes will be applied by the server" );
                    _console->printPrefix();
                }
            }
        } catch( ... ) {
            _console->printDanger( "Server error" );
            _console->printPrefix();
        }
    }
}

#endif
