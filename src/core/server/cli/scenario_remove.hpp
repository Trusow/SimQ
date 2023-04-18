#ifndef SIMQ_CORE_SERVER_CLI_SCENARIO_REMOVE
#define SIMQ_CORE_SERVER_CLI_SCENARIO_REMOVE

#include "scenario.h"
#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "../../../crypto/hash.hpp"
#include "callbacks.hpp"
#include "ini.h"
#include <string.h>
#include <string>
#include <algorithm>
#include <vector>

namespace simq::core::server::CLI {
    class ScenarioRemove: public Scenario {
        private:
            Callbacks *_cb = nullptr;
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            bool _isEnd = false;
            std::string _name = "";

        public:
            ScenarioRemove(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};

            void start();
            void input( std::vector<std::string> &list );
            void password( const char *value ) {};
            void confirm( bool value );
            bool isEnd();
    };

    void ScenarioRemove::start() {
        _isEnd = false;
        _name = "";
    }

    void ScenarioRemove::input( std::vector<std::string> &list ) {
        const char *item = list[1].c_str();

        std::vector<std::string> _items;

        if( _nav->isGroups() ) {
            _cb->getGroups( _items );
        } else if( _nav->isConsumers() ) {
            _cb->getConsumers( _nav->getGroup(), _nav->getChannel(), _items );
        } else if( _nav->isProducers() ) {
            _cb->getProducers( _nav->getGroup(), _nav->getChannel(), _items );
        }

        auto it = std::find( _items.begin(), _items.end(), item );
        if( it == _items.end() ) {
            if( _nav->isGroups() ) {
                _console->printDanger( "Not found group" );
            } else if( _nav->isConsumers() ) {
                _console->printDanger( "Not found consumer" );
            } else if( _nav->isProducers() ) {
                _console->printDanger( "Not found producer" );
            }
            _console->setPrefix( _nav->getPathWithPrefix() );
            _console->printPrefix();
            _isEnd = true;
        } else {
            _name = list[1];
            if( _nav->isGroups() ) {
                _console->confirm( "Removing a group" );
            } else if( _nav->isConsumers() ) {
                _console->confirm( "Removing a consumer" );
            } else if( _nav->isProducers() ) {
                _console->confirm( "Removing a producer" );
            }
        }

    };

    void ScenarioRemove::confirm( bool value ) {
        _isEnd = true;

        if( value == false ) {
            _console->setPrefix( _nav->getPathWithPrefix() );
            _console->printPrefix( true );
            return;
        }

        try {
            if( _nav->isGroups() ) {
                _cb->removeGroup( _name.c_str() );
            } else if( _nav->isConsumers() ) {
                _cb->removeConsumer( _nav->getGroup(), _nav->getChannel(), _name.c_str() );
            } else if( _nav->isProducers() ) {
                _cb->removeProducer( _nav->getGroup(), _nav->getChannel(), _name.c_str() );
            }
            _console->printWarning( "The changes will be applied by the server." );
        } catch( ... ) {
            _console->printDanger( "Server error" );
        }

        _console->setPrefix( _nav->getPathWithPrefix() );
        _console->printPrefix();
    }

    bool ScenarioRemove::isEnd() {
        return _isEnd;
    }
}

#endif
