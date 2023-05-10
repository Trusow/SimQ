#ifndef SIMQ_CORE_SERVER_SESSION
#define SIMQ_CORE_SERVER_SESSION

#include "../../util/error.h"
#include "../../util/string.hpp"
#include "../../util/types.h"
#include <unordered_map>
#include <vector>
#include <time.h>

// NO SAFE THREAD!!!

namespace simq::core::server {
    class Session {
        private:
            const unsigned int DELAY_NO_ACTIVE_SEC = 15;

            struct Item {
                util::types::Initiator initiator;
                char *group;
                char *channel;
                char *login;
                unsigned int ip;
                unsigned int lastTS;
                bool watchTS;
            };

            std::unordered_map<unsigned int, Item> _items;
            void _checkDuplicate( unsigned int fd );
            void _checkIsset( unsigned int fd );

        public:
            void join( unsigned int fd, unsigned int ip );
            void leave( unsigned int fd );

            void setGroupInfo(
                unsigned int fd,
                const char *groupName
            );
            void setConsumerInfo(
                unsigned int fd,
                const char *groupName, const char *channelName, const char *login
            );
            void setProducerInfo(
                unsigned int fd,
                const char *groupName, const char *channelName, const char *login
            );

            const char *getGroup( unsigned int fd );
            const char *getChannel( unsigned int fd );
            const char *getLogin( unsigned int fd );
            unsigned int getIP( unsigned int fd );
            util::types::Initiator getInitiator( unsigned int fd );

            void updateLastTS( unsigned int fd );
            void watchTS( unsigned int fd );
            void unwatchTS( unsigned int fd );
            void getNoActiveList( std::vector<unsigned int> &list );
    };

    void Session::_checkDuplicate( unsigned int fd ) {
        if( _items.find( fd ) != _items.end() ) {
            throw util::Error::DUPLICATE_SESSION;
        }
    }

    void Session::_checkIsset( unsigned int fd ) {
        if( _items.find( fd ) == _items.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }
    }

    void Session::join( unsigned int fd, unsigned int ip ) {
        _checkDuplicate( fd );

        Item item{};
        item.lastTS = time( NULL );
        item.ip = ip;

        _items[fd] = item;
    }

    void Session::setGroupInfo(
        unsigned int fd,
        const char *groupName
    ) {
        _checkIsset( fd );

        Item item{};
        item.lastTS = time( NULL );
        item.initiator = util::types::I_GROUP;
        item.group = util::String::copy( groupName );

        _items[fd] = item;
    }

    void Session::setConsumerInfo(
        unsigned int fd,
        const char *groupName, const char *channelName, const char *login
    ) {
        _checkIsset( fd );

        Item item{};
        item.lastTS = time( NULL );
        item.initiator = util::types::I_CONSUMER;
        item.group = util::String::copy( groupName );
        item.channel = util::String::copy( channelName );
        item.login = util::String::copy( login );

        _items[fd] = item;
    }

    void Session::setProducerInfo(
        unsigned int fd,
        const char *groupName, const char *channelName, const char *login
    ) {
        _checkDuplicate( fd );

        Item item{};
        item.lastTS = time( NULL );
        item.initiator = util::types::I_PRODUCER;
        item.group = util::String::copy( groupName );
        item.channel = util::String::copy( channelName );
        item.login = util::String::copy( login );

        _items[fd] = item;
    }

    void Session::leave( unsigned int fd ) {
        auto it = _items.find( fd );

        if( it == _items.end() ) {
            return;
        }

        auto item = _items[fd];
        
        if( item.group != nullptr ) {
            delete[] item.group;
        }

        if( item.channel != nullptr ) {
            delete[] item.channel;
        }

        if( item.login != nullptr ) {
            delete[] item.login;
        }

        _items.erase( it );
    }

    const char *Session::getGroup( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd].group;
    }

    const char *Session::getChannel( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd].channel;
    }

    const char *Session::getLogin( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd].login;
    }

    unsigned int Session::getIP( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd].ip;
    }

    util::types::Initiator Session::getInitiator( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd].initiator;
    }

    void Session::updateLastTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd].lastTS = time( NULL );
    }

    void Session::watchTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd].watchTS = true;
    }

    void Session::unwatchTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd].watchTS = false;
    }

    void Session::getNoActiveList( std::vector<unsigned int> &list ) {
        auto curTime = time( NULL );
        for( auto it = _items.begin(); it != _items.end(); it++ ) {
            auto item = it->second;

            if( item.watchTS && curTime - item.lastTS > DELAY_NO_ACTIVE_SEC ) {
                list.push_back( it->first );
            }
        }
    }
}

#endif
 
