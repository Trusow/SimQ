#ifndef SIMQ_CORE_SERVER_SESSIONS
#define SIMQ_CORE_SERVER_SESSIONS

#include <vector>
#include <memory>
#include <mutex>
#include "protocol.hpp"
#include "access.hpp"
#include "q/manager.hpp"
#include "fsm.hpp"

namespace simq::core::server {
    class Sessions {
        public:
            enum Type {
                TYPE_COMMON,
                TYPE_GROUP,
                TYPE_CONSUMER,
                TYPE_PRODUCER,
            };
            
            struct Session {
                FSM::Code fsm;
                Type type;
             
                std::unique_ptr<char[]> authData;
                std::unique_ptr<Protocol::Packet> packet;
                std::unique_ptr<Protocol::BasePacket> packetMsg;
                unsigned short int offsetChannel;
                unsigned short int offsetLogin;
             
                unsigned int ip;
                unsigned int lastTS;

                int delayConsumerWait;
             
                unsigned int msgID;
                unsigned int lengthMessage;
                unsigned int sendLengthMessage;
                bool isSignal;
            };

            void _resizeSessions( unsigned int fd );
        private:
            struct WrapperSession {
                unsigned int counter;
                bool isLive;
                std::unique_ptr<Session> sess;
            };

            const unsigned int STEP_RESIZE_SESSIONS = 1024;
            std::vector<std::unique_ptr<WrapperSession>> _sessions;
            std::mutex _mSessions;

            Access *_access = nullptr;
            q::Manager *_q = nullptr;

            void _disconnect( unsigned int fd, WrapperSession *wrapper );
        public:
            Sessions(
                simq::core::server::Access *access,
                simq::core::server::q::Manager *q
            ) : _access{access}, _q{q} {};
            
            Session *connect( unsigned int fd, unsigned int &counter );
            void disconnect( unsigned int fd, unsigned int counter );
    };

    void Sessions::_resizeSessions( unsigned int fd ) {
        while( true ) {
            if( fd < _sessions.size() ) {
                return;
            }
            auto prevSize = _sessions.size();

            _sessions.resize( prevSize + STEP_RESIZE_SESSIONS );

            for( auto i = prevSize; i < prevSize + STEP_RESIZE_SESSIONS; i++ ) {
                _sessions[i] = std::make_unique<WrapperSession>();
            }
        }
    }

    Sessions::Session *Sessions::connect( unsigned int fd, unsigned int &counter ) {
        std::lock_guard<std::mutex> lock( _mSessions );

        if( fd >= _sessions.size() ) {
            _resizeSessions( fd );
        }

        auto wrapper = _sessions[fd].get();

        if( wrapper->isLive ) {
            _disconnect( fd, wrapper );
        }

        auto sess = std::make_unique<Session>();
        sess->fsm = FSM::Code::COMMON_RECV_CMD_CHECK_SECURE;
        sess->type = TYPE_COMMON;
        sess->packet = std::make_unique<Protocol::Packet>();
        sess->packetMsg = std::make_unique<Protocol::BasePacket>();

        wrapper->sess = std::move( sess );
        wrapper->counter++;
        wrapper->isLive = true;
        counter = wrapper->counter;

        return wrapper->sess.get();
    }

    void Sessions::disconnect( unsigned int fd, unsigned int counter ) {
        std::lock_guard<std::mutex> lock( _mSessions );
        auto wrapper = _sessions[fd].get();

        if( !wrapper->isLive || wrapper->counter != counter ) {
            return;
        }

        _disconnect( fd, wrapper );
    }

    void Sessions::_disconnect( unsigned int fd, WrapperSession *wrapper ) {
        auto sess = wrapper->sess.get();
        char *group, *channel, *login;

        switch( sess->type ) {
            case TYPE_GROUP:
                group = sess->authData.get();
                _access->logoutGroup( group, fd );
                break;
            case TYPE_CONSUMER:
                group = sess->authData.get();
                channel = &sess->authData.get()[sess->offsetChannel];
                login = &sess->authData.get()[sess->offsetLogin];

                _access->logoutConsumer( group, channel, login, fd );
                try {
                    if( sess->msgID != 0 ) {
                        if( !sess->isSignal ) {
                            _q->revertMessage( group, channel, fd, sess->msgID );
                        } else {
                            _q->removeMessage( group, channel, fd, sess->msgID );
                        }
                    }
                } catch( ... ) {}
                _q->leaveConsumer( group, channel, fd );
                break;
            case TYPE_PRODUCER:
                group = sess->authData.get();
                channel = &sess->authData.get()[sess->offsetChannel];
                login = &sess->authData.get()[sess->offsetLogin];

                _access->logoutProducer( group, channel, login, fd );
                try {
                    if( sess->msgID != 0 ) {
                        _q->removeMessage( group, channel, fd, sess->msgID );
                    }
                } catch( ... ) {}
                _q->leaveProducer( group, channel, fd );
                break;
        }

        wrapper->isLive = false;
        wrapper->counter++;
    }
}

#endif
 
