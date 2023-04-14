#ifndef SIMQ_CORE_SERVER_CLI
#define SIMQ_CORE_SERVER_CLI

#include "cli_callbacks.hpp"
#include "../../util/string.hpp"
#include "../../crypto/hash.hpp"
#include "../../util/console_callbacks.h"
#include "../../util/console.hpp"
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <iostream>

// TODO Красивый вывод

namespace simq::core::server {
    class CLI: public util::ConsoleCallbacks {
        private:
            CLICallbacks *_cb = nullptr;
            util::Console *_console = nullptr;

            enum ConfirmType {
                CONFIRM_NONE,
                CONFIRM_REMOVE_GROUP,
                CONFIRM_REMOVE_CHANNEL,
                CONFIRM_REMOVE_CONSUMER,
                CONFIRM_REMOVE_PRODUCER,
            };

            enum CtxNavigation {
                CTX_ROOT,

                CTX_GROUPS,
                CTX_GROUP,
                CTX_CHANNEL,
                CTX_CONSUMERS,
                CTX_PRODUCERS,
                CTX_CONSUMER,
                CTX_PRODUCER,

                CTX_SETTINGS,
            };

            struct Navigation {
                CtxNavigation ctx;
                char *group;
                char *channel;
                char *login;
            };

            Navigation *_nav = nullptr;

            std::string _removeName = "";
            ConfirmType _confirmType = CONFIRM_NONE;

            const char *_pathGroups = "groups";
            const char *_pathSettings = "settings";
            const char *_pathConsumers = "consumers";
            const char *_pathProducers = "producers";

            const char *_warningChanges = "The changes will be applied by the server.";
            const char *_errNotFoundPath = "Not found path.";

            const char *_defPrefix = "simq";

            const char *_cmdLs = "ls";
            const char *_cmdH = "h";
            const char *_cmdCd = "cd";
            const char *_cmdInfo = "info";
            const char *_cmdSet = "set";
            const char *_cmdPswd = "passwd";
            const char *_cmdRemove = "rm";
            const char *_cmdAdd = "add";

            void _getCtxPath( std::string &path, CtxNavigation ctx );
            void _getLS( Navigation *nav, std::vector<std::string> &list );
            void _getAllowedCommands( std::vector<std::string> &list );

            Navigation *_buildNavigation( const char *line );
            void _goNext( Navigation *nav, const char *path );
            void _goBack( Navigation *nav );
            void _goRoot( Navigation *nav );
            void _freeNavigation( Navigation *nav );
            void _applyNavigation( Navigation *target, Navigation *src );

            void _printHelp( std::vector<std::string> &allowedCommands );
            void _rm( std::vector<std::string> &list );
            void _cd( std::vector<std::string> &list );
            void _ls( std::vector<std::string> &list );

            void _printConsolePrefix( bool isNewLine = false );

            void _remove( const char *name );
            void _removeConfirm( const char *name );

        public:
            CLI( CLICallbacks *cb );
            ~CLI();
            void input( std::vector<std::string> &list );
            void inputPassword( const char *password );
            void confirm( bool value );
    };

    CLI::CLI( CLICallbacks *cb ) {
        _cb = cb;

        _nav = new Navigation{};
        _nav->ctx = CTX_GROUPS;

        _console = new util::Console( this );
        _console->getPassword( "Input password to continue: " );
        _console->run();
    }

    CLI::~CLI() {
        delete this->_console;
    }

    void CLI::_getCtxPath( std::string &path, CtxNavigation ctx ) {
        switch( ctx ) {
            case CTX_ROOT:
                break;
            case CTX_GROUPS:
                path += _pathGroups;
                break;
            case CTX_GROUP:
                _getCtxPath( path, CTX_GROUPS );
                path += "/";
                path += _nav->group;
                break;
            case CTX_CHANNEL:
                _getCtxPath( path, CTX_GROUP );
                path += "/";
                path += _nav->channel;
                break;
            case CTX_CONSUMERS:
                _getCtxPath( path, CTX_CHANNEL );
                path += "/";
                path += _pathConsumers;
                break;
            case CTX_PRODUCERS:
                _getCtxPath( path, CTX_CHANNEL );
                path += "/";
                path += _pathProducers;
                break;
            case CTX_CONSUMER:
                _getCtxPath( path, CTX_CONSUMERS );
                path += "/";
                path += _nav->login;
                break;
            case CTX_PRODUCER:
                _getCtxPath( path, CTX_PRODUCERS );
                path += "/";
                path += _nav->login;
                break;
            case CTX_SETTINGS:
                path += _pathSettings;
                break;
        }
    }

    void CLI::_getLS( Navigation *nav, std::vector<std::string> &list ) {
        switch( nav->ctx ) {
            case CTX_ROOT:
                list.push_back( _pathGroups );
                list.push_back( _pathSettings );
                break;
            case CTX_GROUPS:
                _cb->getGroups( list );
                break;
            case CTX_GROUP:
                _cb->getChannels( nav->group, list );
                break;
            case CTX_CHANNEL:
                list.push_back( _pathConsumers );
                list.push_back( _pathProducers );
                break;
            case CTX_CONSUMERS:
                _cb->getConsumers( nav->group, nav->channel, list );
                break;
            case CTX_PRODUCERS:
                _cb->getProducers( nav->group, nav->channel, list );
                break;
        }
    }

    void CLI::_getAllowedCommands( std::vector<std::string> &list ) {
        list.push_back( _cmdH );
        list.push_back( _cmdCd );
        switch( _nav->ctx ) {
            case CTX_ROOT:
                list.push_back( _cmdLs );
                break;
            case CTX_GROUPS:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdRemove );
                break;
            case CTX_GROUP:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdRemove );
                list.push_back( _cmdPswd );
                break;
            case CTX_CHANNEL:
                list.push_back( _cmdLs );
                list.push_back( _cmdInfo );
                list.push_back( _cmdSet );
                break;
            case CTX_CONSUMERS:
            case CTX_PRODUCERS:
                list.push_back( _cmdLs );
                list.push_back( _cmdAdd );
                list.push_back( _cmdRemove );
                break;
            case CTX_PRODUCER:
            case CTX_CONSUMER:
                list.push_back( _cmdPswd );
                break;
            case CTX_SETTINGS:
                list.push_back( _cmdInfo );
                list.push_back( _cmdSet );
                list.push_back( _cmdPswd );
        }
    }

    void CLI::_applyNavigation( Navigation *target, Navigation *src ) {
        target->ctx = src->ctx;

        util::String::free( target->group );
        util::String::free( target->channel );
        util::String::free( target->login );

        target->group = util::String::copy( src->group );
        target->channel = util::String::copy( src->channel );
        target->login = util::String::copy( src->login );
    }

    void CLI::_goRoot( Navigation *nav ) {
        nav->ctx = CTX_ROOT;

        util::String::free( nav->group );
        util::String::free( nav->channel );
        util::String::free( nav->login );
        nav->group = nullptr;
        nav->channel = nullptr;
        nav->login = nullptr;
    }

    void CLI::_goBack( Navigation *nav ) {
        switch( nav->ctx ) {
            case CTX_GROUPS:
            case CTX_SETTINGS:
                _goRoot( nav );
                break;
            case CTX_GROUP:
                util::String::free( nav->group );
                util::String::free( nav->channel );
                util::String::free( nav->login );
                nav->group = nullptr;
                nav->channel = nullptr;
                nav->login = nullptr;
                nav->ctx = CTX_GROUPS;
                break;
            case CTX_CHANNEL:
                // TODO detect isset group
                util::String::free( nav->channel );
                nav->channel = nullptr;
                nav->ctx = CTX_GROUP;
                break;
            case CTX_CONSUMERS:
            case CTX_PRODUCERS:
                // TODO detect isset group
                nav->ctx = CTX_CHANNEL;
                break;
            case CTX_CONSUMER:
                // TODO detect isset group
                util::String::free( nav->login );
                nav->login = nullptr;
                nav->ctx = CTX_CONSUMERS;
                break;
            case CTX_PRODUCER:
                // TODO detect isset group
                util::String::free( nav->login );
                nav->login = nullptr;
                nav->ctx = CTX_PRODUCERS;
                break;
        }
    }

    void CLI::_goNext( Navigation *nav, const char *path ) {
        std::vector<std::string> list;
        _getLS( nav, list );

        auto it = std::find( list.begin(), list.end(), path );
        if( it == list.end() ) {
            throw -1;
        }

        switch( nav->ctx ) {
            case CTX_ROOT:
                if( std::string( path ) == _pathGroups ) {
                    nav->ctx = CTX_GROUPS;
                } else {
                    nav->ctx = CTX_SETTINGS;
                }
                break;
            case CTX_GROUPS:
                nav->ctx = CTX_GROUP;
                nav->group = util::String::copy( path );
                break;
            case CTX_GROUP:
                nav->ctx = CTX_CHANNEL;
                nav->channel = util::String::copy( path );
                break;
            case CTX_CHANNEL:
                if( std::string( path ) == _pathConsumers ) {
                    nav->ctx = CTX_CONSUMERS;
                } else {
                    nav->ctx = CTX_PRODUCERS;
                }
                break;
            case CTX_CONSUMERS:
                nav->ctx = CTX_CONSUMER;
                nav->login = util::String::copy( path );
                break;
            case CTX_PRODUCERS:
                nav->ctx = CTX_PRODUCER;
                nav->login = util::String::copy( path );
                break;
            default:
                throw -1;
        }
    }

    CLI::Navigation *CLI::_buildNavigation( const char *line ) {
        // TODO дописать CD

        std::string path = "";
        bool isSlash = false;
        unsigned int point = 0;
        std::vector<std::string> list;

        auto localNav = new Navigation{};
        _applyNavigation( localNav, _nav );

        for( unsigned int i = 0; line[i] != 0; i++ ) {
            char ch = line[i];

            if( ch == '/' ) {
                if( i == 0 ) {
                    _goRoot( localNav );
                } else if( path != "" ) {
                    _goNext( localNav, path.c_str() );
                }

                path = "";
                point = 0;
                isSlash = true;
            } else if( ch == '.' ) {
                if( path != "" || point == 2 ) {
                    throw -1;
                }
                path = "";
                point++;
                isSlash = false;
                if( point == 2 ) {
                    _goBack( localNav );
                }
            } else {
                if( point != 0 ) {
                    throw -1;
                }
                path += ch;
                isSlash = false;
            }
        }

        if( path != "" ) {
            _goNext( localNav, path.c_str() );
        }

        return localNav;
    }

    void CLI::_removeConfirm( const char *name ) {
        _removeName = name;

        switch( _nav->ctx ) {
            case CTX_GROUPS:
                _confirmType = CONFIRM_REMOVE_GROUP;
                _console->confirm( "Removing a group" );
                break;
            case CTX_GROUP:
                _confirmType = CONFIRM_REMOVE_CHANNEL;
                _console->confirm( "Removing a channel" );
                break;
            case CTX_CONSUMERS:
                _confirmType = CONFIRM_REMOVE_CONSUMER;
                _console->confirm( "Removing a consumer" );
                break;
            case CTX_PRODUCERS:
                _confirmType = CONFIRM_REMOVE_PRODUCER;
                _console->confirm( "Removing a producer" );
                break;
        }

    }

    void CLI::_remove( const char *name ) {
        try {
            switch( _nav->ctx ) {
                case CTX_GROUPS:
                    _cb->removeGroup( name );
                    break;
                case CTX_CHANNEL:
                    _cb->removeChannel( _nav->group, name );
                    break;
                case CTX_CONSUMERS:
                    _cb->removeConsumer( _nav->group, _nav->channel, name );
                    break;
                case CTX_PRODUCERS:
                    _cb->removeProducer( _nav->group, _nav->channel, name );
                    break;
            }

            _console->printWarning( _warningChanges );
            _console->printPrefix();
        } catch( ... ) {
            _console->printDanger( "Wrong name" );
            _console->printPrefix();
        }
    }

    void CLI::_printHelp( std::vector<std::string> &allowedCommands ) {
        std::vector<std::string> help;

        for( unsigned int i = 0; i < allowedCommands.size(); i++ ) {
            std::string cmd = allowedCommands[i];
            if( cmd == _cmdH ) {
                continue;
            }

            std::string str = cmd;

            if( cmd == _cmdAdd ) {
                switch( _nav->ctx ) {
                    case CTX_GROUPS:
                        str += " - create group, example '";
                        str += cmd + " groupTest'";
                        break;
                    case CTX_GROUP:
                        str += " - create channel, example '";
                        str += cmd + " channelTest'";
                        break;
                    case CTX_CONSUMERS:
                        str += " - create consumer, example '";
                        str += cmd + " consumerTest'";
                        break;
                    case CTX_PRODUCERS:
                        str += " - create producer, example '";
                        str += cmd + " producerTest'";
                        break;
                }
            } else if( cmd == _cmdRemove ) {
                switch( _nav->ctx ) {
                    case CTX_GROUPS:
                        str += " - remove group, example '";
                        str += cmd + " groupTest'";
                        break;
                    case CTX_GROUP:
                        str += " - remove channel, example '";
                        str += cmd + " channelTest'";
                        break;
                    case CTX_CONSUMERS:
                        str += " - remove consumer, example '";
                        str += cmd + " consumerTest'";
                        break;
                    case CTX_PRODUCERS:
                        str += " - remove producer, example '";
                        str += cmd + " producerTest'";
                        break;
                }
            } else if( cmd == _cmdPswd ) {
                switch( _nav->ctx ) {
                    case CTX_GROUP:
                        str += " - update password of group";
                        break;
                    case CTX_CONSUMER:
                        str += " - update password of consumer";
                        break;
                    case CTX_PRODUCER:
                        str += " - update password of producer";
                        break;
                    case CTX_SETTINGS:
                        str += " - update password of server";
                        break;
                }
            } else if( cmd == _cmdInfo ) {
                switch( _nav->ctx ) {
                    case CTX_SETTINGS:
                        str += " - show settings of server";
                        break;
                    case CTX_CHANNEL:
                        str += " - show settings of channel";
                        break;
                }
            } else if( cmd == _cmdSet ) {
                switch( _nav->ctx ) {
                    case CTX_SETTINGS:
                        str += " - save settings of server, example '";
                        str += cmd + " nameField valueField'";
                        break;
                    case CTX_CHANNEL:
                        str += " - save settings of server, example '";
                        str += cmd + " nameField valueField'";
                        break;
                }
            } else if( cmd == _cmdLs ) {
                str += " - show available paths, is set filter example '";
                str += cmd + " query'";
            } else if( cmd == _cmdCd ) {
                str += " - go to path";
            }

            help.push_back( str );

        }
        
        _console->printList( help );
        _console->printPrefix();
    }

    void CLI::_cd( std::vector<std::string> &list ) {
        if( list.size() != 2 ) {
            _console->printDanger( "Wrong param" );
            _console->printPrefix();
            return;
        }

        try {
            auto nav = _buildNavigation( list[1].c_str() );
            _applyNavigation( _nav, nav );
            _printConsolePrefix( true );
        } catch( ... ) {
            _console->printDanger( _errNotFoundPath );
            _console->printPrefix();
        }
    }

    void CLI::_rm( std::vector<std::string> &list ) {
        if( list.size() != 2 ) {
            _console->printDanger( "Wrong params" );
            _console->printPrefix();
            return;
        }

        _removeConfirm( list[1].c_str() );
    }

    void CLI::_ls( std::vector<std::string> &list ) {
        try {
            std::vector<std::string> lsList;
            _getLS( _nav, lsList );

            if( list.size() > 1 ) {
                _console->printList( lsList, list[1].c_str() );
            } else {
                _console->printList( lsList );
            }
        } catch( ... ) {
            _console->printDanger( _errNotFoundPath );
        }

        _console->printPrefix();
    }

    void CLI::input( std::vector<std::string> &list ) {
        auto cmd = list[0];

        std::vector<std::string> allowedCommands;
        _getAllowedCommands( allowedCommands );

        auto it = std::find( allowedCommands.begin(), allowedCommands.end(), cmd );

        if( it == allowedCommands.end() ) {
            _console->printDanger( "Unknown command" );
            _console->printPrefix();
        } else if( cmd == _cmdLs ) {
            _ls( list );
        } else if( cmd == _cmdCd ) {
            _cd( list );
        } else if( cmd == _cmdH ) {
            _printHelp( allowedCommands );
        } else if( cmd == _cmdRemove ) {
            _rm( list );
        }
    }

    void CLI::inputPassword( const char *password ) {
        unsigned char hash[simq::crypto::HASH_LENGTH];
        unsigned char hashOrig[simq::crypto::HASH_LENGTH];
        simq::crypto::Hash::hash( password, hash );

        _cb->getMasterPassword( hashOrig );

        if( strncmp(
            ( const char * )hash,
            ( const char * )hashOrig,
            simq::crypto::HASH_LENGTH
        ) != 0 ) {
            _console->printDanger( "Error password" );
            _console->exit();
        } else {
            _console->printSuccess( "Welcome to SimQ - a simple message queue.\nAuthor - Sergej Trusow, version - 1.0\nPress 'h' to help." );
            _printConsolePrefix();
        }

    }

    void CLI::confirm( bool value ) {
        if( !value ) {
            _console->printWarning( "The changes will not be applied" );
            _console->printPrefix();
            return;
        }

        switch( _confirmType ) {
            case CONFIRM_REMOVE_GROUP:
            case CONFIRM_REMOVE_CHANNEL:
            case CONFIRM_REMOVE_CONSUMER:
            case CONFIRM_REMOVE_PRODUCER:
                _remove( _removeName.c_str() );
                break;
            default:
                _console->printPrefix( true );
        }
    }

    void CLI::_printConsolePrefix( bool isNewLine ) {
        std::string path = _defPrefix;
        path += ": /";
        _getCtxPath( path, _nav->ctx );
        path += "> ";

        _console->setPrefix( path.c_str() );
        _console->printPrefix( isNewLine );
    }
}

#endif
