#ifndef SIMQ_CORE_SERVER_CLI_SCENARIO_AUTH
#define SIMQ_CORE_SERVER_CLI_SCENARIO_AUTH

#include "scenario.h"
#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "../../../crypto/hash.hpp"
#include "callbacks.hpp"
#include "ini.h"
#include <string.h>
#include <string>
#include <vector>

namespace simq::core::server::CLI {
    class ScenarioAuth: public Scenario {
        private:
            Callbacks *_cb = nullptr;
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            bool _isEnd = false;

        public:
            ScenarioAuth(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};

            void start();
            void input( std::vector<std::string> &list ) {};
            void password( const char *value );
            void confirm( bool value ) {};
            bool isEnd();
    };

    void ScenarioAuth::start() {
        _isEnd = false;
        _console->printSuccess( "Welcome to SimQ - a simple message queue.\nAuthor - Sergej Trusow, version - 1.0\nPress 'h' to help." );
        _console->getPassword( "Input password to continue: " );
    }

    void ScenarioAuth::password( const char *value ) {
        _isEnd = true;
        const auto l = crypto::HASH_LENGTH;
        unsigned char origHashPswd[l];
        unsigned char hashPswd[l];
        
        _cb->getMasterPassword( origHashPswd );
        crypto::Hash::hash( value, hashPswd );

        if( strncmp(
            ( const char * )origHashPswd,
            ( const char * )hashPswd,
            l
        ) != 0 ) {
            _console->printDanger( "Wrong password" );
            _console->exit();
        } else {
            _console->setPrefix( _nav->getPathWithPrefix() );
            _console->printPrefix( true );
        }
    }

    bool ScenarioAuth::isEnd() {
        return _isEnd;
    }
}

#endif
