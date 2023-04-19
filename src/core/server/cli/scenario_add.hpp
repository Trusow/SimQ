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

    void ScenarioAdd::input( std::vector<std::string> &list ) {

        if( _step == STEP_NONE ) {

            const char *item = list[1].c_str();
            
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

            auto it = std::find( _items.begin(), _items.end(), item );
            if( it != _items.end() ) {
                if( _nav->isGroups() ) {
                    _console->printDanger( "Duplicate group" );
                } else if( _nav->isGroup() ) {
                    _console->printDanger( "Duplicate channel" );
                } else if( _nav->isConsumers() ) {
                    _console->printDanger( "Duplicate consumer" );
                } else if( _nav->isProducers() ) {
                    _console->printDanger( "Duplicate producer" );
                }
                _console->setPrefix( _nav->getPathWithPrefix() );
                _console->printPrefix();
                _isEnd = true;
            } else {
                _name = list[1];

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
            if( list.empty() ) {
                _console->printDanger( "Empty params" );
                _console->printPrefix();
            } else if( list.size() > 1 ) {
                _console->printDanger( "Many params" );
                _console->printPrefix();
            } else {
                unsigned int value = atol( list[0].c_str() );
                if( !util::Validation::isUInt( list[0].c_str() ) ) {
                    _console->printDanger( "Wrong param" );
                    _console->printPrefix();
                } else {
                    if( _step == STEP_CH_MIN_MESSAGE_SIZE ) {
                        if( value == 0 ) {
                            _console->printDanger( "Wrong param" );
                            _console->printPrefix();
                        } else {
                            _channelSettings.minMessageSize = value;
                            _step = STEP_CH_MAX_MESSAGE_SIZE;
                            _console->setPrefix( "Max message size: " );
                            _console->printPrefix( true );
                        }
                    } else if( _step == STEP_CH_MAX_MESSAGE_SIZE ) {
                        if( value == 0 ) {
                            _console->printDanger( "Wrong param" );
                            _console->printPrefix();
                        } else {
                            _channelSettings.maxMessageSize = value;
                            if( !util::Validation::isChannelSettings( &_channelSettings ) ) {
                                _console->printDanger( "Wrong param" );
                                _console->printPrefix();
                            } else {
                                _channelSettings.maxMessageSize = value;
                                _step = STEP_CH_MAX_MESSAGES_IN_MEMORY;
                                _console->setPrefix( "Max messages in memory: " );
                                _console->printPrefix( true );
                            }
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
                                _console->printWarning( "The changes will be applied by the server." );
                            } catch( ... ) {
                                _console->printDanger( "Server error" );
                            }
                            _console->setPrefix( _nav->getPathWithPrefix() );
                            _console->printPrefix();
                            _console->enableHistory();
                        }
                    }
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
                _console->printDanger( "Passwords don't match" );
                _console->setPrefix( _nav->getPathWithPrefix() );
                _console->printPrefix();
                _isEnd = true;
            } else {
                try {
                    if( _nav->isGroups() ) {
                        _cb->addGroup( _name.c_str(), hashPswd );
                        _console->printWarning( "The changes will be applied by the server." );
                        _console->setPrefix( _nav->getPathWithPrefix() );
                        _console->printPrefix();
                        _isEnd = true;
                    } else if( _nav->isProducers() ) {
                        _cb->addConsumer(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _name.c_str(),
                            hashPswd
                        );
                        _console->printWarning( "The changes will be applied by the server." );
                        _console->setPrefix( _nav->getPathWithPrefix() );
                        _console->printPrefix();
                        _isEnd = true;
                    } else if( _nav->isConsumers() ) {
                        _cb->addProducer(
                            _nav->getGroup(),
                            _nav->getChannel(),
                            _name.c_str(),
                            hashPswd
                        );
                        _console->printWarning( "The changes will be applied by the server." );
                        _console->setPrefix( _nav->getPathWithPrefix() );
                        _console->printPrefix();
                        _isEnd = true;
                    }
                } catch( ... ) {
                    _console->printDanger( "Server error" );
                    _console->setPrefix( _nav->getPathWithPrefix() );
                    _console->printPrefix();
                    _isEnd = true;
                }
            }
        }
    }

    bool ScenarioAdd::isEnd() {
        return _isEnd;
    }
}

#endif
