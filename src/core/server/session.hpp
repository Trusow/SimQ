#ifndef SIMQ_CORE_SERVER_SESSION
#define SIMQ_CORE_SERVER_SESSION

#include "../../util/error.h"
#include "../../util/string.hpp"
#include "../../util/types.h"
#include <unordered_map>
#include <vector>
#include <time.h>
#include <memory>

// NO SAFE THREAD!!!

namespace simq::core::server {
    class Session {
        private:
            const unsigned int DELAY_NO_ACTIVE_SEC = 15;

            struct Item {
                util::types::Initiator initiator;
                std::unique_ptr<char[]> group;
                std::unique_ptr<char[]> channel;
                std::unique_ptr<char[]> login;
                unsigned int ip;
                unsigned int lastTS;
                bool watchTS;
            };

            std::unordered_map<unsigned int, std::unique_ptr<Item>> _items;
            void _checkDuplicate( unsigned int fd );
            void _checkIsset( unsigned int fd );
            void _copyGroup( Item *item, const char *name );
            void _copyChannel( Item *item, const char *name );
            void _copyLogin( Item *item, const char *name );

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

        auto item = std::make_unique<Item>();
        item->lastTS = time( NULL );
        item->watchTS = true;
        item->ip = ip;

        _items[fd] = std::move( item );
    }

    void Session::_copyGroup( Item *item, const char *name ) {
        auto l = strlen( name );
        item->group = std::make_unique<char[]>( l + 1 );
        memcpy( item->group.get(), name, l );
    }

    void Session::_copyChannel( Item *item, const char *name ) {
        auto l = strlen( name );
        item->channel = std::make_unique<char[]>( l + 1 );
        memcpy( item->channel.get(), name, l );
    }

    void Session::_copyLogin( Item *item, const char *name ) {
        auto l = strlen( name );
        item->login = std::make_unique<char[]>( l + 1 );
        memcpy( item->login.get(), name, l );
    }

    void Session::setGroupInfo(
        unsigned int fd,
        const char *groupName
    ) {
        _checkIsset( fd );
        auto item = _items[fd].get();

        item->lastTS = time( NULL );
        item->initiator = util::types::I_GROUP;

        _copyGroup( item, groupName );
    }

    void Session::setConsumerInfo(
        unsigned int fd,
        const char *groupName, const char *channelName, const char *login
    ) {
        _checkIsset( fd );
        auto item = _items[fd].get();

        item->lastTS = time( NULL );
        item->initiator = util::types::I_CONSUMER;

        _copyGroup( item, groupName );
        _copyChannel( item, channelName );
        _copyLogin( item, login );
    }

    void Session::setProducerInfo(
        unsigned int fd,
        const char *groupName, const char *channelName, const char *login
    ) {
        _checkDuplicate( fd );
        auto item = _items[fd].get();

        item->lastTS = time( NULL );
        item->initiator = util::types::I_PRODUCER;

        _copyGroup( item, groupName );
        _copyChannel( item, channelName );
        _copyLogin( item, login );
    }

    void Session::leave( unsigned int fd ) {
        auto it = _items.find( fd );

        if( it == _items.end() ) {
            return;
        }

        _items.erase( it );
    }

    const char *Session::getGroup( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd]->group.get();
    }

    const char *Session::getChannel( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd]->channel.get();
    }

    const char *Session::getLogin( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd]->login.get();
    }

    unsigned int Session::getIP( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd]->ip;
    }

    util::types::Initiator Session::getInitiator( unsigned int fd ) {
        _checkIsset( fd );

        return _items[fd]->initiator;
    }

    void Session::updateLastTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd]->lastTS = time( NULL );
    }

    void Session::watchTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd]->watchTS = true;
    }

    void Session::unwatchTS( unsigned int fd ) {
        _checkIsset( fd );

        _items[fd]->watchTS = false;
    }

    void Session::getNoActiveList( std::vector<unsigned int> &list ) {
        auto curTime = time( NULL );
        for( auto it = _items.begin(); it != _items.end(); it++ ) {
            auto item = it->second.get();

            if( item->watchTS && curTime - item->lastTS > DELAY_NO_ACTIVE_SEC ) {
                list.push_back( it->first );
            }
        }
    }
}

#endif
 
