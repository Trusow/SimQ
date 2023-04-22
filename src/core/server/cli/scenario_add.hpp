#ifndef SIMQ_CORE_SERVER_CLI_SCENARIO_ADD
#define SIMQ_CORE_SERVER_CLI_SCENARIO_ADD

#include "scenario.h"
#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "../../../util/types.h"
#include "../../../util/validation.hpp"
#include "../../../crypto/hash.hpp"
#include "callbacks.hpp"
#include "ini.h"
#include <string.h>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <vector>

namespace simq::core::server::CLI {
    class ScenarioAdd: public Scenario {
        private:
            Callbacks *_cb = nullptr;
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            bool _isEnd = false;

            std::string _name = "";
            std::string _password = "";
            std::string _confirmPassword = "";

            util::Types::ChannelSettings _channelSettings;

            enum Step {
                STEP_NONE,
                STEP_CH_MIN_MESSAGE_SIZE,
                STEP_CH_MAX_MESSAGE_SIZE,
                STEP_CH_MAX_MESSAGES_IN_MEMORY,
                STEP_CH_MAX_MESSAGES_ON_DISK,
            };

            Step _step = STEP_NONE;

            bool _isDuplicate( const char *name );
            void _printDuplicateError();
            bool _validateChannelParam( std::vector<std::string> &list );
            bool _validateChannelMinMessageSize( unsigned int value );
            bool _validateChannelMaxMessageSize( unsigned int value, unsigned int min );


        public:
            ScenarioAdd(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};

            void start();
            void input( std::vector<std::string> &list );
            void password( const char *value );
            void confirm( bool value ) {};
            bool isEnd();
    };

    void ScenarioAdd::start() {
        _isEnd = false;
        _name = "";
        _password = "";
        _confirmPassword = "";
        _step = STEP_NONE;
        memset( &_channelSettings, 0, sizeof( util::Types::ChannelSettings ) );
        // Нужно чтобы проходило валидацию при заполнении
        _channelSettings.maxMessagesInMemory = 1;
    }

    bool ScenarioAdd::_isDuplicate( const char *name ) {
        std::vector<std::string> _items;

        if( _nav->isGroups() ) {
            _cb->getGroups( _items );
        } else if( _nav->isGroup() ) {
            _cb->getChannels( _nav->getGroup(), _items );
        } else if( _nav->isConsumers() ) {
            _cb->getConsumers( _nav->getGroup(), _nav->getChannel(), _items );
        } else if( _nav->isProducers() ) {
            _cb->getProducers( _nav->getGroup(), _nav->getChannel(), _items );
        }

        auto it = std::find( _items.begin(), _items.end(), name );

        return it != _items.end();
    }

    void ScenarioAdd::_printDuplicateError() {
        if( _nav->isGroups() ) {
            Ini::printDanger( _console, "Duplicate group", _nav->getPathWithPrefix() );
        } else if( _nav->isGroup() ) {
            Ini::printDanger( _console, "Duplicate channel", _nav->getPathWithPrefix() );
        } else if( _nav->isConsumers() ) {
            Ini::printDanger( _console, "Duplicate consumer", _nav->getPathWithPrefix() );
        } else if( _nav->isProducers() ) {
            Ini::printDanger( _console, "Duplicate producer", _nav->getPathWithPrefix() );
        }
    }

    bool ScenarioAdd::_validateChannelParam( std::vector<std::string> &list ) {
        if( list.empty() ) {
            Ini::printDanger( _console, "Empty params" );
            return false;
        } else if( list.size() > 1 ) {
            Ini::printDanger( _console, "Many params" );
            return false;
        } else if( !util::Validation::isUInt( list[0].c_str() ) ) {
            Ini::printDanger( _console, "Wrong param" );
            return false;
        }

        return true;
    }

    bool ScenarioAdd::_validateChannelMinMessageSize( unsigned int value ) {
        if( value == 0 ) {
            Ini::printDanger( _console, "Wrong param" );
            return false;
        }

        return true;
    }

    bool ScenarioAdd::_validateChannelMaxMessageSize( unsigned int value, unsigned int min ) {
        if( value == 0 || min > value ) {
            Ini::printDanger( _console, "Wrong param" );
            return false;
        }

        return true;
    }

    void ScenarioAdd::input( std::vector<std::string> &list ) {

        if( _step == STEP_NONE ) {
            if( list.empty() ) {
                Ini::printDanger( _console, "Empty params", _nav->getPathWithPrefix() );
                _isEnd = true;
                return;
            }

            if( _isDuplicate( list[0].c_str() ) ) {
                _printDuplicateError();
                _isEnd = true;
            } else {
                _name = list[0];

                if( _nav->isGroup() ) {
                    _console->disableHistory();
                    _console->setPrefix( "Min message size: " );
                    _console->printPrefix( true );
                    _step = STEP_CH_MIN_MESSAGE_SIZE;
                } else {
                    _console->getPassword( "Input password:" );
                }
            }
        } else if(
            _step == STEP_CH_MIN_MESSAGE_SIZE ||
            _step == STEP_CH_MAX_MESSAGE_SIZE ||
            _step == STEP_CH_MAX_MESSAGES_IN_MEMORY ||
            _step == STEP_CH_MAX_MESSAGES_ON_DISK
        ) {
            if( !_validateChannelParam( list ) ) {
                return;
            }

            unsigned int value = atol( list[0].c_str() );

            if( _step == STEP_CH_MIN_MESSAGE_SIZE ) {
                if( _validateChannelMinMessageSize( value ) ) {
                    _channelSettings.minMessageSize = value;
                    _step = STEP_CH_MAX_MESSAGE_SIZE;
                    _console->setPrefix( "Max message size: " );
                    _console->printPrefix( true );
                }
            } else if( _step == STEP_CH_MAX_MESSAGE_SIZE ) {
                if( _validateChannelMaxMessageSize( value, _channelSettings.minMessageSize ) ) {
                    _channelSettings.maxMessageSize = value;
                    _step = STEP_CH_MAX_MESSAGES_IN_MEMORY;
                    _console->setPrefix( "Max messages in memory: " );
                    _console->printPrefix( true );
                }
            } else if( _step == STEP_CH_MAX_MESSAGES_IN_MEMORY ) {
                _channelSettings.maxMessagesInMemory = value;
                _step = STEP_CH_MAX_MESSAGES_ON_DISK;
                _console->setPrefix( "Max messages on disk: " );
                _console->printPrefix( true );
            } else if( _step == STEP_CH_MAX_MESSAGES_ON_DISK ) {
                _channelSettings.maxMessagesOnDisk = value;
                if( !util::Validation::isChannelSettings( &_channelSettings ) ) {
                    _console->printDanger( "Wrong param" );
                    _console->printPrefix();
                } else {
                    _isEnd = true;
                    try {
                        _cb->addChannel( _nav->getGroup(), _name.c_str(), &_channelSettings );
                        Ini::printWarning( _console, Ini::msgApplyChangesDefer, _nav->getPathWithPrefix() );
                    } catch( ... ) {
                        Ini::printDanger( _console, "Server error", _nav->getPathWithPrefix() );
                    }
                    _console->enableHistory();
                }
            }
        }

    };

    void ScenarioAdd::password( const char *value ) {
        const auto l = crypto::HASH_LENGTH;
        unsigned char hashPswd[l];
        crypto::Hash::hash( value, hashPswd );

        if( _password == "" && value[0] == 0 ) {
            _console->getPassword( "Input password:" );
            return;
        } else if( _password == "" ) {
            _password = value;
            _console->getPassword( "Confirm password:" );
        } else if( _confirmPassword == "" ) {
            _confirmPassword = value;

            if( _password != _confirmPassword ) {
                Ini::printDanger( _console, "Passwords don't match", _nav->getPathWithPrefix() );
                _isEnd = true;
            } else {
                try {
                    if( _nav->isGroups() ) {
                        _cb->addGroup( _name.c_str(), hashPswd );
                    } else if( _nav->isProducers() ) {
                        _cb->addConsumer(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _name.c_str(),
                            hashPswd
                        );
                    } else if( _nav->isConsumers() ) {
                        _cb->addProducer(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _name.c_str(),
                            hashPswd
                        );
                    }
                    Ini::printWarning( _console, Ini::msgApplyChangesDefer, _nav->getPathWithPrefix() );
                } catch( ... ) {
                    Ini::printDanger( _console, "Server error", _nav->getPathWithPrefix() );
                }

                _isEnd = true;
            }
        }
    }

    bool ScenarioAdd::isEnd() {
        return _isEnd;
    }
}

#endif
