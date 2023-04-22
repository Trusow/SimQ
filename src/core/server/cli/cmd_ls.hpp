#ifndef SIMQ_CORE_SERVER_CLI_CMD_LS
#define SIMQ_CORE_SERVER_CLI_CMD_LS

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "callbacks.hpp"
#include "cmd.h"
#include "ini.h"
#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class CmdLs: public Cmd {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            Callbacks *_cb = nullptr;
        public:
            CmdLs(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};
            void run( std::vector<std::string> &params );
    };

    void CmdLs::run( std::vector<std::string> &params ) {
        if( params.size() > 1 ) {
            _console->printDanger( "Many params" );
            _console->printPrefix();
            return;
        }

        std::vector<std::string> list;

        try {
            if( _nav->isRoot() ) {
                list.push_back( Ini::pathGroups );
                list.push_back( Ini::pathSettings );
            } else if( _nav->isGroups() ) {
                _cb->getGroups( list );
            } else if( _nav->isGroup() ) {
                _cb->getChannels( _nav->getGroup(), list );
            } else if( _nav->isChannel() ) {
                list.push_back( Ini::pathConsumers );
                list.push_back( Ini::pathProducers );
            } else if( _nav->isConsumers() ) {
                _cb->getConsumers( _nav->getGroup(), _nav->getChannel(), list );
            } else if( _nav->isProducers() ) {
                _cb->getProducers( _nav->getGroup(), _nav->getChannel(), list );
            }

            if( params.empty() ) {
                _console->printList( list );
            } else {
                _console->printList( list, params[0].c_str() );
            }

        } catch( ... ) {
            _console->printDanger( "Not found path" );
        }
        _console->printPrefix();
    }
}

#endif
