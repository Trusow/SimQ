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
        if( params.empty() ) {
            Ini::printDanger( _console, "Not found name of param" );
            return;
        } else if( params.size() == 1 ) {
            Ini::printDanger( _console, "Not found value of param" );
            return;
        } else if( params.size() > 2 ) {
            Ini::printDanger( _console, "ManyParams" );
            return;
        }

        unsigned int num = 0;
        bool isChannel = false;
        util::Types::ChannelSettings settings;

        auto name = params[0];
        auto value = params[1].c_str();

        if( value[0] != 0 && !util::Validation::isUInt( value ) ) {
            Ini::printDanger( _console, "Wrong value" );
            return;
        } else {
            num = atol( value );
        }

        try {
            if( name == Ini::infoSettingsPort ) {
                if( !util::Validation::isPort( num ) ) {
                    Ini::printDanger( _console, "Wrong value" );
                } else {
                    _cb->updatePort( num );
                    Ini::printSuccess( _console, "Restart server to apply changes" );
                }
            } else if( name == Ini::infoSettingsCountThreads ) {
                if( !util::Validation::isCountThread( num ) ) {
                    Ini::printDanger( _console, "Wrong value", _nav->getPathWithPrefix() );
                } else {
                    _cb->updateCountThreads( num );
                    Ini::printSuccess( _console, "Restart server to apply changes" );
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
                Ini::printDanger( _console, "Unknown name" );
            }

            if( isChannel ) {
                if( !util::Validation::isChannelSettings( &settings ) ) {
                    Ini::printDanger( _console, "Wrong value" );
                } else {
                    Ini::printWarning( _console, Ini::msgApplyChangesDefer );
                }
            }
        } catch( ... ) {
            Ini::printDanger( _console, "Server error" );
        }
    }
}

#endif
