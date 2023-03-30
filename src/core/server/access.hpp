#ifndef SIMQ_CORE_SERVER_ACCESS
#define SIMQ_CORE_SERVER_ACCESS

#include "../../util/lock_atomic.hpp"
#include "../../util/error.h"
#include "../../crypto/hash.hpp"
#include <atomic>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <map>
#include <list>
#include <deque>
#include <string.h>
#include <iostream>
#include <cstring>
#include <string>

namespace simq::core::server {
    class Access {
        private:
        std::atomic_uint countGroupWrited {0};
        std::atomic<unsigned int> countSessWrited {0};
        std::atomic<unsigned int> countChannelWrited {0};
        std::atomic<unsigned int> countConsumerWrited {0};
        std::atomic<unsigned int> countProducerWrited {0};
        std::shared_timed_mutex mGroup;
        std::shared_timed_mutex mSess;
        std::shared_timed_mutex mChannel;
        std::shared_timed_mutex mConsumer;
        std::shared_timed_mutex mProducer;

        enum SessionType {
            GROUP,
            CONSUMER,
            PRODUCER,
            NONE,
        };

        struct Session {
            char *data;
            unsigned int offsetChannel;
            unsigned int offsetLogin;
            SessionType type;
        };

        struct Entity {
            unsigned char password[crypto::HASH_LENGTH];
            std::deque<unsigned int> sessions;
        };

        struct Group: Entity {};
        struct Consumer: Entity {};
        struct Producer: Entity {};

        std::map<unsigned int, Session> sessions;
        std::map<std::string, Group> groups;
        std::map<std::string, std::map<std::string, bool>> channels;
        std::map<std::string, std::map<std::string, std::map<std::string, Consumer>>> consumers;
        std::map<std::string, std::map<std::string, std::map<std::string, Producer>>> producers;

        void _clearGroupSession( const char *name, unsigned int fd );
        void _clearConsumerSession( const char *group, const char *channel, const char *name, unsigned int fd );
        void _clearProducerSession( const char *group, const char *channel, const char *name, unsigned int fd );
        void _addChannel( const char *group, const char *name );
        void _removeChannel( const char *group, const char *name );
        void _addConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void _removeConsumer( const char *group, const char *channel, const char *name );
        void _addProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void _removeProducer( const char *group, const char *channel, const char *name );
        void _validateConsumer( const char *group, const char *channel, const char *name );
        void _validateProducer( const char *group, const char *channel, const char *name );
        bool _validateSessGroup( unsigned int fd, std::string &groupName );
        bool _validateIssetGroup( unsigned int fd, const char *group );
        bool _validateIssetChannel( unsigned int fd, const char *group, const char *channel );
        void wait( std::atomic_uint &atom );
     
        public:
        void addGroup( const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void authGroup( const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd );
        void removeGroup( const char *name );
     
        void addChannel( const char *group, const char *name );
        void addChannel( unsigned int fd, const char *name );
        void removeChannel( const char *group, const char *name );
        void removeChannel( unsigned int fd, const char *name );
     
        void addConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void addConsumer( unsigned int fd, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void authConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd );
        void removeConsumer( const char *group, const char *channel, const char *name );
        void removeConsumer( unsigned int fd, const char *channel, const char *name );
     
        void addProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void addProducer( unsigned int fd, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] );
        void authProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd );
        void removeProducer( const char *group, const char *channel, const char *name );
        void removeProducer( unsigned int fd, const char *channel, const char *name );

        void updatePassword( unsigned int fd, const unsigned char oldPassword[crypto::HASH_LENGTH], const unsigned char newPassword[crypto::HASH_LENGTH] );
        void updateConsumerPassword( unsigned int fd, const char *channel, const char *name, const unsigned char newPassword[crypto::HASH_LENGTH] );
        void updateProducerPassword( unsigned int fd, const char *channel, const char *name, const unsigned char newPassword[crypto::HASH_LENGTH] );
     
        void logout( unsigned int fd );
     
        bool canSendMessage( unsigned int fd );
        bool canRecvMessage( unsigned int fd );

        bool canCreateChannel( unsigned int fd, const char *channel );
        bool canRemoveChannel( unsigned int fd, const char *channel );

        bool canCreateConsumer( unsigned int fd, const char *channel, const char *name );
        bool canUpdateConsumerPassword( unsigned int fd, const char *channel, const char *name );
        bool canRemoveConsumer( unsigned int fd, const char *channel, const char *name );

        bool canCreateProducer( unsigned int fd, const char *channel, const char *name );
        bool canUpdateProducerPassword( unsigned int fd, const char *channel, const char *name );
        bool canRemoveProducer( unsigned int fd, const char *channel, const char *name );
    };

    void Access::wait( std::atomic_uint &atom ) {
        while( atom ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }
 
    void Access::addGroup( const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        util::LockAtomic lockAtomicGroup( countGroupWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );


        if( groups.count( name ) ) {
            throw util::Error::DUPLICATE_GROUP;
        }

        auto group = Group();
        memcpy( group.password, password, crypto::HASH_LENGTH );
        groups[name] = group;
    }
     
    void Access::removeGroup( const char *name ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        {
            util::LockAtomic lockAtomicGroup( countGroupWrited );
            std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );

            auto iter = groups.find( name );
            if( iter == groups.end() ) {
                throw util::Error::NOT_FOUND_GROUP;
            }

            for( auto iterSess: groups[name].sessions ) {
                sessions[iterSess].type = SessionType::NONE;
            }

            groups.erase( iter );
        }

        {
            util::LockAtomic lockAtomicChannel( countChannelWrited );
            std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

            auto iterChannel = channels.find( name );
            if( iterChannel != channels.end() ) {
                channels.erase( iterChannel );
            }
        }

        {
            util::LockAtomic lockAtomicConsumer( countConsumerWrited );
            std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

            auto iterConsumer = consumers.find( name );
            if( iterConsumer != consumers.end() ) {
                for( auto itChannel = consumers[name].begin(); itChannel != consumers[name].end(); itChannel++ ) {
                    for( auto itConsumer = consumers[name][itChannel->first].begin(); itConsumer != consumers[name][itChannel->first].end(); itConsumer++ ) {
                        for( auto itSess: itConsumer->second.sessions ) {
                            sessions[itSess].type = SessionType::NONE;
                        }
                    }
                }
                consumers.erase( iterConsumer );
            }
        }
        
        {
            util::LockAtomic lockAtomicProducer( countProducerWrited );
            std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

            auto iterProducer = producers.find( name );
            if( iterProducer != producers.end() ) {
                for( auto itChannel = producers[name].begin(); itChannel != producers[name].end(); itChannel++ ) {
                    for( auto itProducer = producers[name][itChannel->first].begin(); itProducer != producers[name][itChannel->first].end(); itProducer++ ) {
                        for( auto itSess: itProducer->second.sessions ) {
                            sessions[itSess].type = SessionType::NONE;
                        }
                    }
                }
                producers.erase( iterProducer );
            }

        }
     

    }
     
    void Access::authGroup( const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        util::LockAtomic lockAtomicGroup( countGroupWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );

        auto iter = groups.find( name );
        if( iter == groups.end() ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( strncmp( ( const char * )groups[name].password, ( const char * )password, crypto::HASH_LENGTH ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        auto sess = Session();
        sess.type = SessionType::GROUP;

        auto length = strlen( name );
        sess.data = new char[length+1]{};
        memcpy( sess.data, name, length );

        auto iterSess = sessions.find( fd );

        if( iterSess != sessions.end() ) {
            throw util::Error::DUPLICATE_SESSION;
        }

        sessions[fd] = sess;

        groups[name].sessions.push_back( fd );
    }

    void Access::_addChannel( const char *group, const char *name ) {
        {
            wait( countGroupWrited );
            std::shared_lock<std::shared_timed_mutex> lockGroup( mGroup );

            if( !groups.count( group ) ) {
                throw util::Error::NOT_FOUND_GROUP;
            }
        }

        util::LockAtomic lockAtomicChannel( countChannelWrited );
        std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

        auto iterGroup = channels.find( std::string( group ) );

        if( iterGroup == channels.end() ) {
            channels[group][name] = true;
        } else if( !iterGroup->second.count( name ) ) {
            iterGroup->second[name] = true;
        } else {
            throw util::Error::DUPLICATE_CHANNEL;
        }
    }
     
    void Access::addChannel( const char *group, const char *name ) {
        _addChannel( group, name );
    }

    void Access::addChannel( unsigned int fd, const char *name ) {
        std::string groupName;

        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            if( !sessions.count( fd ) ) {
                throw util::Error::NOT_FOUND_SESSION;
            }

            auto sess = sessions[fd];
            if( sess.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }
            groupName = sess.data;
        }

        _addChannel( groupName.c_str(), name );
    }

    void Access::_removeChannel( const char *group, const char *name ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        {
            util::LockAtomic lockAtomicChannel( countChannelWrited );
            std::lock_guard<std::shared_timed_mutex> lockChannel( mChannel );

            auto iterGroup = channels.find( group );
            if( iterGroup == channels.end() ) {
                throw util::Error::NOT_FOUND_GROUP;
            }

            auto iterChannel = iterGroup->second.find( name );
            if( iterChannel != iterGroup->second.end() ) {
                iterGroup->second.erase( iterChannel );
            } else {
                throw util::Error::NOT_FOUND_CHANNEL;
            }
        }

        {
            util::LockAtomic lockAtomicConsumer( countConsumerWrited );
            std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

            auto iterConsumer = consumers.find( group );
            if( iterConsumer != consumers.end() ) {
                auto iterChannel = iterConsumer->second.find( name );
                if( iterChannel != iterConsumer->second.end() ) {
                    for( auto itConsumer = iterChannel->second.begin(); itConsumer != iterChannel->second.end(); itConsumer++ ) {
                        for( auto itSess: itConsumer->second.sessions ) {
                            sessions[itSess].type = SessionType::NONE;
                        }
                    }

                    consumers[group].erase( iterChannel );
                }

            }
        }
        
        {
            util::LockAtomic lockAtomicProducer( countProducerWrited );
            std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

            auto iterProducer = producers.find( group );
            if( iterProducer != producers.end() ) {
                auto iterChannel = iterProducer->second.find( name );
                if( iterChannel != iterProducer->second.end() ) {
                    for( auto itProducer = iterChannel->second.begin(); itProducer != iterChannel->second.end(); itProducer++ ) {
                        for( auto itSess: itProducer->second.sessions ) {
                            sessions[itSess].type = SessionType::NONE;
                        }
                    }
                    producers[group].erase( iterChannel );
                }
            }

        }
    }
     
     
    void Access::removeChannel( const char *group, const char *name ) {
        _removeChannel( group, name );
    }

    void Access::removeChannel( unsigned int fd, const char *name ) {
        std::string groupName;
        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            if( !sessions.count( fd ) ) {
                throw util::Error::NOT_FOUND_SESSION;
            }

            auto sess = sessions[fd];
            if( sess.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }
            groupName = sess.data;
        }

        _removeChannel( groupName.c_str(), name );
    }

    void Access::_addConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {

        {
            wait( countChannelWrited );
            std::shared_lock<std::shared_timed_mutex> lockChannel( mChannel );

            auto iterGroup = channels.find( group );
            if( iterGroup == channels.end() ) {
                throw util::Error::NOT_FOUND_GROUP;
            }

            if( !iterGroup->second.count( channel ) ) {
                throw util::Error::NOT_FOUND_CHANNEL;
            }
        }

        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

        auto consumer = Consumer();
        memcpy( consumer.password, password, crypto::HASH_LENGTH );

        if( !consumers.count( group ) || !consumers[group].count( channel ) || !consumers[group][channel].count( name ) ) {
            consumers[group][channel][name] = consumer;
        } else {
            throw util::Error::DUPLICATE_CONSUMER;
        }

    }

    void Access::_addProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        {
            wait( countChannelWrited );
            std::shared_lock<std::shared_timed_mutex> lockChannel( mChannel );

            auto iterGroup = channels.find( group );
            if( iterGroup == channels.end() ) {
                throw util::Error::NOT_FOUND_GROUP;
            }

            if( !iterGroup->second.count( channel ) ) {
                throw util::Error::NOT_FOUND_CHANNEL;
            }
        }

        util::LockAtomic lockAtomicConsumer( countProducerWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

        auto producer = Producer();
        memcpy( producer.password, password, crypto::HASH_LENGTH );

        if( !producers.count( group ) || !producers[group].count( channel ) || !producers[group][channel].count( name ) ) {
            producers[group][channel][name] = producer;
        } else {
            throw util::Error::DUPLICATE_PRODUCER;
        }

    }
     
    void Access::addConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        _addConsumer( group, channel, name, password );
    }

    void Access::addConsumer( unsigned int fd, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        std::string groupName;

        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            if( !sessions.count( fd ) ) {
                throw util::Error::NOT_FOUND_SESSION;
            }

            auto sess = sessions[fd];
            if( sess.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }
            groupName = sess.data;
        }

        _addConsumer( groupName.c_str(), channel, name, password );
    }


    void Access::_removeConsumer( const char *group, const char *channel, const char *name ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

        if( !consumers.count(group) || !consumers[group].count( channel ) || !consumers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        auto iterConsumer = consumers[group][channel].find( name );

        for( auto it = iterConsumer->second.sessions.begin(); it != iterConsumer->second.sessions.end(); it++ ) {
            sessions[*it].type = SessionType::NONE;
        }

        consumers[group][channel].erase( iterConsumer );
    }

    void Access::_removeProducer( const char *group, const char *channel, const char *name ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        util::LockAtomic lockAtomicProducer( countProducerWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

        if( !producers.count(group) || !producers[group].count( channel ) || !producers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_PRODUCER;
        }

        auto iterProducer = producers[group][channel].find( name );

        for( auto it = iterProducer->second.sessions.begin(); it != iterProducer->second.sessions.end(); it++ ) {
            sessions[*it].type = SessionType::NONE;
        }

        producers[group][channel].erase( iterProducer );
    }

     
     
    void Access::removeConsumer( const char *group, const char *channel, const char *name ) {
        _removeConsumer( group, channel, name );
    }

    void Access::removeConsumer( unsigned int fd, const char *channel, const char *name ) {
        std::string group;

        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            auto iterSess = sessions.find( fd );
            if( iterSess == sessions.end() ) {
                throw util::Error::NOT_FOUND_SESSION;
            } else if( iterSess->second.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }

            group = iterSess->second.data;
        }

        _removeConsumer( group.c_str(), channel, name );
    }
     
    void Access::authConsumer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        if( sessions.count( fd ) ) {
            throw util::Error::DUPLICATE_SESSION;
        }

        auto sess = Session();
        sess.type = SessionType::CONSUMER;

        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( mConsumer );

        if( !consumers.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !consumers[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        } else if( !consumers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        auto iterConsumer = consumers[group][channel].find( name );
        if( strncmp( ( char *)iterConsumer->second.password, ( char *)password, crypto::HASH_LENGTH ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        auto lengthGroup = strlen( group );
        auto lengthChannel = strlen( channel );
        auto lengthName = strlen( name );
        auto allLength = lengthGroup + lengthChannel + lengthName + 3;
        sess.data = new char[allLength]{};
        memcpy( sess.data, group, lengthGroup );
        memcpy( &sess.data[lengthGroup+1], channel, lengthChannel );
        memcpy( &sess.data[lengthGroup+lengthChannel+2], name, lengthName );
        sess.offsetChannel = lengthGroup + 1;
        sess.offsetLogin = lengthGroup + lengthName + 2;

        consumers[group][channel][name].sessions.push_back( fd );
        sessions[fd] = sess;
    }
     
    void Access::addProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        _addProducer( group, channel, name, password );
    }

    void Access::addProducer( unsigned int fd, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH] ) {
        std::string groupName;

        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            if( !sessions.count( fd ) ) {
                throw util::Error::NOT_FOUND_SESSION;
            }

            auto sess = sessions[fd];
            if( sess.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }
            groupName = sess.data;
        }

        _addProducer( groupName.c_str(), channel, name, password );
    }
     
    void Access::removeProducer( const char *group, const char *channel, const char *name ) {
        _removeProducer( group, channel, name );
    }

    void Access::removeProducer( unsigned int fd, const char *channel, const char *name ) {
        std::string group;

        {
            wait( countSessWrited );
            std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

            auto iterSess = sessions.find( fd );
            if( iterSess == sessions.end() ) {
                throw util::Error::NOT_FOUND_SESSION;
            } else if( iterSess->second.type != SessionType::GROUP ) {
                throw util::Error::ACCESS_DENY;
            }

            group = iterSess->second.data;
        }

        _removeProducer( group.c_str(), channel, name );
    }
     
    void Access::authProducer( const char *group, const char *channel, const char *name, const unsigned char password[crypto::HASH_LENGTH], unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        if( sessions.count( fd ) ) {
            throw util::Error::DUPLICATE_SESSION;
        }

        auto sess = Session();
        sess.type = SessionType::PRODUCER;

        util::LockAtomic lockAtomicProducer( countProducerWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

        if( !producers.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !producers[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        } else if( !producers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }

        auto iterProducer = producers[group][channel].find( name );
        if( strncmp( ( char *)iterProducer->second.password, ( char *)password, crypto::HASH_LENGTH ) != 0 ) {
            throw util::Error::WRONG_PASSWORD;
        }

        auto lengthGroup = strlen( group );
        auto lengthChannel = strlen( channel );
        auto lengthName = strlen( name );
        auto allLength = lengthGroup + lengthChannel + lengthName + 3;
        sess.data = new char[allLength]{};
        sess.offsetChannel = lengthGroup + 1;
        sess.offsetLogin = lengthGroup + lengthName + 2;
        memcpy( sess.data, group, lengthGroup );
        memcpy( &sess.data[sess.offsetChannel], channel, lengthChannel );
        memcpy( &sess.data[sess.offsetLogin], name, lengthName );

        producers[group][channel][name].sessions.push_back( fd );
        sessions[fd] = sess;
    }

    void Access::_clearGroupSession( const char *name, unsigned int fd ) {
        util::LockAtomic lockAtomicGroup( countGroupWrited );
        std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );

        auto iter = groups.find( name );

        if( iter == groups.end() ) {
            return;
        }

        for( auto iterSess = groups[name].sessions.begin(); iterSess != groups[name].sessions.end(); ) {
            if( *iterSess == fd ) {
                groups[name].sessions.erase( iterSess );
                break;
            } else {
                iterSess++;
            }
        }
    }

    void Access::_clearConsumerSession( const char *group, const char *channel, const char *name, unsigned int fd ) {
        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

        if( !consumers.count( group ) || !consumers[group].count( channel ) || !consumers[group][channel].count( name ) ) {
            return;
        }

        for( auto iterSess = consumers[group][channel][name].sessions.begin(); iterSess != consumers[group][channel][name].sessions.end(); ) {
            if( *iterSess == fd ) {
                consumers[group][channel][name].sessions.erase( iterSess );
                break;
            } else {
                iterSess++;
            }
        }
    }

    void Access::_clearProducerSession( const char *group, const char *channel, const char *name, unsigned int fd ) {
        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConcumer( mConsumer );

        if( !producers.count( group ) || !producers[group].count( channel ) || !producers[group][channel].count( name ) ) {
            return;
        }

        for( auto iterSess = producers[group][channel][name].sessions.begin(); iterSess != producers[group][channel][name].sessions.end(); ) {
            if( *iterSess == fd ) {
                producers[group][channel][name].sessions.erase( iterSess );
                break;
            } else {
                iterSess++;
            }
        }
    }
     
    void Access::logout( unsigned int fd ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        auto iter = sessions.find( fd );
        if( iter == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }

        Consumer consumer;
        Producer producer;

        auto type = sessions[fd].type;

        if( type == SessionType::GROUP ) {
            _clearGroupSession( iter->second.data, fd );
        } else if( type == SessionType::CONSUMER ) {
            auto val = iter->second;
            _clearConsumerSession( val.data, &val.data[val.offsetChannel], &val.data[val.offsetLogin], fd );
        } else if( type == SessionType::PRODUCER ) {
            auto val = iter->second;
            _clearProducerSession( val.data, &val.data[val.offsetChannel], &val.data[val.offsetLogin], fd );
        }
        delete[] sessions[fd].data;
        sessions.erase( iter );
    }
     
    bool Access::canSendMessage( unsigned int fd ) {
        wait( countSessWrited );
        std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

        auto iterSess = sessions.find( fd );
        if( iterSess == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }

     
        return iterSess->second.type == SessionType::PRODUCER;
    }
     
    bool Access::canRecvMessage( unsigned int fd ) {
        wait( countSessWrited );
        std::shared_lock<std::shared_timed_mutex> lockSess( mSess );
     
     
        auto iterSess = sessions.find( fd );
        if( iterSess == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        }

     
        return iterSess->second.type == SessionType::CONSUMER;
    }

    bool Access::canCreateChannel( unsigned int fd, const char *channel ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        if( !_validateIssetGroup( fd, groupName.c_str() ) ) {
            return false;
        }

        return !_validateIssetChannel( fd, groupName.c_str(), channel );
    }

    bool Access::canRemoveChannel( unsigned int fd, const char *channel ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        return _validateIssetChannel( fd, groupName.c_str(), channel );
    }

    bool Access::canCreateConsumer( unsigned int fd, const char *channel, const char *name ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        const char *group = groupName.c_str();

        if( !_validateIssetChannel( fd, group, channel ) ) {
            return false;
        }

        wait( countConsumerWrited );
        std::shared_lock<std::shared_timed_mutex> lockConcumer( mConsumer );

        if( !consumers.count( group ) || !consumers[group].count( channel ) || !consumers[group][channel].count( name ) ) {
            return true;
        }

        return false;
    }

    bool Access::canUpdateConsumerPassword( unsigned int fd, const char *channel, const char *name ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        const char *group = groupName.c_str();

        if( !_validateIssetChannel( fd, group, channel ) ) {
            return false;
        }

        wait( countConsumerWrited );
        std::shared_lock<std::shared_timed_mutex> lockConcumer( mConsumer );

        if( !consumers.count( group ) || !consumers[group].count( channel ) || !consumers[group][channel].count( name ) ) {
            return false;
        }

        return true;
    }

    bool Access::canRemoveConsumer( unsigned int fd, const char *channel, const char *name ) {
        return canUpdateConsumerPassword( fd, channel, name );
    }

    bool Access::canCreateProducer( unsigned int fd, const char *channel, const char *name ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        const char *group = groupName.c_str();

        if( !_validateIssetChannel( fd, group, channel ) ) {
            return false;
        }

        wait( countProducerWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( mProducer );

        if( !producers.count( group ) || !producers[group].count( channel ) || !producers[group][channel].count( name ) ) {
            return true;
        } else {
            return false;
        }
    }

    bool Access::canUpdateProducerPassword( unsigned int fd, const char *channel, const char *name ) {
        std::string groupName;

        if( !_validateSessGroup( fd, groupName ) ) {
            return false;
        }

        const char *group = groupName.c_str();

        if( !_validateIssetChannel( fd, group, channel ) ) {
            return false;
        }

        wait( countProducerWrited );
        std::shared_lock<std::shared_timed_mutex> lockProducer( mProducer );

        if( !producers.count( group ) || !producers[group].count( channel ) || !producers[group][channel].count( name ) ) {
            return false;
        }

        return true;
    }

    bool Access::canRemoveProducer( unsigned int fd, const char *channel, const char *name ) {
        return canUpdateProducerPassword( fd, channel, name );
    }

    void Access::_validateConsumer( const char *group, const char *channel, const char *name ) {
        if( !consumers.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !consumers[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        } else if( !consumers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }
    }

    void Access::_validateProducer( const char *group, const char *channel, const char *name ) {
        if( !producers.count( group ) ) {
            throw util::Error::NOT_FOUND_GROUP;
        } else if( !producers[group].count( channel ) ) {
            throw util::Error::NOT_FOUND_CHANNEL;
        } else if( !producers[group][channel].count( name ) ) {
            throw util::Error::NOT_FOUND_CONSUMER;
        }
    }

    bool Access::_validateSessGroup( unsigned int fd, std::string &groupName ) {
        wait( countSessWrited );
        std::shared_lock<std::shared_timed_mutex> lockSess( mSess );

        if( !sessions.count( fd ) ) {
            return false;
        }

        auto sess = sessions[fd];
        if( sess.type != SessionType::GROUP ) {
            return false;
        }
        groupName = sess.data;

        return true;
    }

    bool Access::_validateIssetGroup( unsigned int fd, const char *group ) {
        wait( countGroupWrited );
        std::shared_lock<std::shared_timed_mutex> lockGroup( mGroup );

        return groups.count( group ) == 1;
    }

    bool Access::_validateIssetChannel( unsigned int fd, const char *group, const char *channel ) {
        wait( countChannelWrited );
        std::shared_lock<std::shared_timed_mutex> lockChannel( mChannel );

        auto iterGroup = channels.find( group );
        if( iterGroup == channels.end() ) {
            return false;
        }

        if( !iterGroup->second.count( channel ) ) {
            return false;
        }
        
        return true;
    }


    void Access::updatePassword( unsigned int fd, const unsigned char oldPassword[crypto::HASH_LENGTH], const unsigned char newPassword[crypto::HASH_LENGTH] ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        auto iterSess = sessions.find( fd );
        if( iterSess == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        } else if( iterSess->second.type == SessionType::NONE ) {
            throw util::Error::ACCESS_DENY;
        } else if( iterSess->second.type == SessionType::CONSUMER ) {
            util::LockAtomic lockAtomicConsumer( countConsumerWrited );
            std::lock_guard<std::shared_timed_mutex> lockConsumer( mConsumer );

            const char *group = iterSess->second.data;
            const char *channel = &iterSess->second.data[iterSess->second.offsetChannel];
            const char *name = &iterSess->second.data[iterSess->second.offsetLogin];

            _validateConsumer( group, channel, name );
            if( strncmp( ( char *)consumers[group][channel][name].password, ( char *)oldPassword, crypto::HASH_LENGTH ) != 0 ) {
                throw util::Error::WRONG_PASSWORD;
            }

            memcpy( consumers[group][channel][name].password, newPassword, crypto::HASH_LENGTH );

            for( auto it = consumers[group][channel][name].sessions.begin(); it != consumers[group][channel][name].sessions.end(); it++ ) {
                if( sessions.count( *it ) && *it != fd ) {
                    sessions[*it].type = SessionType::NONE;
                }
            }

            consumers[group][channel][name].sessions.clear();
            consumers[group][channel][name].sessions.push_back( fd );

        } else if( iterSess->second.type == SessionType::PRODUCER ) {
            util::LockAtomic lockAtomicProducer( countProducerWrited );
            std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

            const char *group = iterSess->second.data;
            const char *channel = &iterSess->second.data[iterSess->second.offsetChannel];
            const char *name = &iterSess->second.data[iterSess->second.offsetLogin];

            std::cout << group << std::endl;
            std::cout << channel << std::endl;
            std::cout << name << std::endl;
            _validateProducer( group, channel, name );
            if( strncmp( ( char *)producers[group][channel][name].password, ( char *)oldPassword, crypto::HASH_LENGTH ) != 0 ) {
                throw util::Error::WRONG_PASSWORD;
            }

            memcpy( producers[group][channel][name].password, newPassword, crypto::HASH_LENGTH );

            for( auto it = producers[group][channel][name].sessions.begin(); it != producers[group][channel][name].sessions.end(); it++ ) {
                if( sessions.count( *it ) && *it != fd ) {
                    sessions[*it].type = SessionType::NONE;
                }
            }

            producers[group][channel][name].sessions.clear();
            producers[group][channel][name].sessions.push_back( fd );

        } else if( iterSess->second.type == SessionType::GROUP ) {
            util::LockAtomic lockAtomicGroup( countGroupWrited );
            std::lock_guard<std::shared_timed_mutex> lockGroup( mGroup );
            
            const char *group = iterSess->second.data;

            if( !groups.count( group ) ) {
                throw util::Error::NOT_FOUND_GROUP;
            } else  if( strncmp( ( char *)groups[group].password, ( char *)oldPassword, crypto::HASH_LENGTH ) != 0 ) {
                throw util::Error::WRONG_PASSWORD;
            }

            memcpy( groups[group].password, newPassword, crypto::HASH_LENGTH );

            for( auto it = groups[group].sessions.begin(); it != groups[group].sessions.end(); it++ ) {
                if( sessions.count( *it ) && *it != fd ) {
                    sessions[*it].type = SessionType::NONE;
                }
            }

            groups[group].sessions.clear();
            groups[group].sessions.push_back( fd );
        }
    }

    void Access::updateConsumerPassword( unsigned int fd, const char *channel, const char *name, const unsigned char newPassword[crypto::HASH_LENGTH] ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        auto iterSess = sessions.find( fd );
        if( iterSess == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        } else if( iterSess->second.type != SessionType::GROUP ) {
            throw util::Error::ACCESS_DENY;
        }

        util::LockAtomic lockAtomicConsumer( countConsumerWrited );
        std::lock_guard<std::shared_timed_mutex> lockConsumer( mConsumer );

        const char *group = iterSess->second.data;

        _validateConsumer( group, channel, name );
        memcpy( consumers[group][channel][name].password, newPassword, crypto::HASH_LENGTH );

        for( auto it = consumers[group][channel][name].sessions.begin(); it != consumers[group][channel][name].sessions.end(); it++ ) {
            if( sessions.count( *it ) ) {
                sessions[*it].type = SessionType::NONE;
            }
        }

        consumers[group][channel][name].sessions.clear();
    }

    void Access::updateProducerPassword( unsigned int fd, const char *channel, const char *name, const unsigned char newPassword[crypto::HASH_LENGTH] ) {
        util::LockAtomic lockAtomicSess( countSessWrited );
        std::lock_guard<std::shared_timed_mutex> lockSess( mSess );

        auto iterSess = sessions.find( fd );
        if( iterSess == sessions.end() ) {
            throw util::Error::NOT_FOUND_SESSION;
        } else if( iterSess->second.type != SessionType::GROUP ) {
            throw util::Error::ACCESS_DENY;
        }

        util::LockAtomic lockAtomicProducer( countProducerWrited );
        std::lock_guard<std::shared_timed_mutex> lockProducer( mProducer );

        const char *group = iterSess->second.data;

        _validateProducer( group, channel, name );
        memcpy( producers[group][channel][name].password, newPassword, crypto::HASH_LENGTH );

        for( auto it = producers[group][channel][name].sessions.begin(); it != producers[group][channel][name].sessions.end(); it++ ) {
            if( sessions.count( *it ) ) {
                sessions[*it].type = SessionType::NONE;
            }
        }

        producers[group][channel][name].sessions.clear();
    }
}

#endif
