#ifndef SIMQ_CORE_SERVER_CLI_LS
#define SIMQ_CORE_SERVER_CLI_LS

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "callbacks.hpp"
#include "ini.h"
#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class Ls {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            Callbacks *_cb = nullptr;
        public:
            Ls(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};
            void print( const char *query = nullptr );
    };

    void Ls::print( const char *query ) {
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

            _console->printList( list, query );

        } catch( ... ) {
            _console->printDanger( "Not found path" );
        }
        _console->printPrefix();
    }
}

#endif
