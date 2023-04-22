#ifndef SIMQ_CORE_SERVER_CLI_SCENARIO_PASSWORD
#define SIMQ_CORE_SERVER_CLI_SCENARIO_PASSWORD

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
    class ScenarioPassword: public Scenario {
        private:
            Callbacks *_cb = nullptr;
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            bool _isEnd = false;
            bool _isCorrectCurrentPassword = false;
            std::string _newPassword = "";
            std::string _confirmPassword = "";

        public:
            ScenarioPassword(
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

    void ScenarioPassword::start() {
        _isEnd = false;
        _newPassword = "";
        _confirmPassword = "";
        _isCorrectCurrentPassword = false;

        if( _nav->isSettings() ) {
            _console->getPassword( "Input current password: " );
        } else {
            _console->getPassword( "Input new password: " );
        }

    }

    void ScenarioPassword::password( const char *value ) {
        const auto l = crypto::HASH_LENGTH;
        unsigned char origHashPswd[l];
        unsigned char hashPswd[l];

        crypto::Hash::hash( value, hashPswd );

        if( _nav->isSettings() && !_isCorrectCurrentPassword ) {
            _cb->getMasterPassword( origHashPswd );
            if( strncmp( ( const char * )origHashPswd, ( const char * )hashPswd, l ) != 0 ) {
                Ini::printDanger( _console, "Wrong password", _nav->getPathWithPrefix() );
                _isEnd = true;
            } else {
                _isCorrectCurrentPassword = true;
                _console->getPassword( "Input new password: " );
            }
        } else if( _newPassword == "" ) {
            if( value[0] == 0 ) {
                _console->getPassword( "Input new password: " );
                return;
            } else {
                _console->getPassword( "Confirm password: " );
                _newPassword = value;
            }
        } else if( _confirmPassword == "" ) {
            _confirmPassword = value;
            if( _newPassword != _confirmPassword ) {
                Ini::printDanger( _console, "Passwords don't match", _nav->getPathWithPrefix() );
                _isEnd = true;
            } else {
                try {
                    if( _nav->isSettings() ) {
                        _cb->updateMasterPassword( hashPswd );
                        _console->printSuccess( "The password has been changed. Please log in again" );
                        _console->exit();
                    } else if( _nav->isGroup() ) {
                        _cb->updateGroupPassword( _nav->getGroup(), hashPswd );
                        Ini::printWarning( _console, Ini::msgApplyChangesDefer, _nav->getPathWithPrefix() );
                    } else if( _nav->isConsumer() ) {
                        _cb->updateConsumerPassword(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _nav->getUser(),
                            hashPswd
                        );
                        Ini::printWarning( _console, Ini::msgApplyChangesDefer, _nav->getPathWithPrefix() );
                    } else if( _nav->isProducer() ) {
                        _cb->updateProducerPassword(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _nav->getUser(),
                            hashPswd
                        );
                        Ini::printWarning( _console, Ini::msgApplyChangesDefer, _nav->getPathWithPrefix() );
                    }
                } catch( ... ) {
                    Ini::printDanger( _console, "Server error", _nav->getPathWithPrefix() );
                }
            }
            _isEnd = true;
        }
    }

    bool ScenarioPassword::isEnd() {
        return _isEnd;
    }
}

#endif
