#ifndef SIMQ_CORE_SERVER_CLI_INFO
#define SIMQ_CORE_SERVER_CLI_INFO

#include "navigation.hpp"
#include "../../../util/console.hpp"
#include "../../../util/types.h"
#include "callbacks.hpp"
#include <vector>
#include <string>

namespace simq::core::server::CLI {
    class Info {
        private:
            Navigation *_nav = nullptr;
            util::Console *_console = nullptr;
            Callbacks *_cb = nullptr;
            void _addToList( std::vector<std::string> &list, const char *name, unsigned int value );
        public:
            Info(
                util::Console *console,
                Navigation *nav,
                Callbacks *cb
            ) : _console{console}, _nav{nav}, _cb{cb} {};
            void print();
    };

    void Info::_addToList( std::vector<std::string> &list, const char *name, unsigned int value ) {
        std::string item = name;
        item += " - ";
        item += std::to_string( value );
        list.push_back( item );
    }

    void Info::print() {
        std::vector<std::string> list;

        if( _nav->isSettings() ) {
            _addToList( list, "port", _cb->getPort() );
            _addToList( list, "countThreads", _cb->getCountThreads() );
        } else if( _nav->isChannel() ) {
            util::Types::ChannelSettings settings;
            _cb->getChannelSettings(
                _nav->getGroup(),
                _nav->getChannel(),
                settings
            );
            _addToList( list, "minMessageSize", settings.minMessageSize );
            _addToList( list, "maxMessageSize", settings.maxMessageSize );
            _addToList( list, "maxMessagesInMemory", settings.maxMessagesInMemory );
            _addToList( list, "maxMessagesOnDisk", settings.maxMessagesOnDisk );
        }

        _console->printList( list );
        _console->printPrefix();
    }
}

#endif
