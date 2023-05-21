#ifndef SIMQ_CORE_SERVER_SERVER_CONTROLLER
#define SIMQ_CORE_SERVER_SERVER_CONTROLLER

#include <unordered_map>
#include <memory>
#include <vector>
#include <unistd.h>
#include "server/callbacks.h"
#include "../../util/error.h"
#include "../../util/types.h"
#include "access.hpp"
#include "changes.hpp"
#include "q/manager.hpp"
#include "protocol.hpp"

// NO SAFE THREAD!!!

namespace simq::core::server {
    class ServerController: public server::Callbacks {
        private:
            enum TypeRecv {
                RECV_CHECK_SECURE,
                RECV_GET_VERSION,
                RECV_AUTH,
                RECV_CMD_GROUP,
                RECV_CMD_CONSUMER,
                RECV_CMD_PRODUCER,

                RECV_CMD_CONSUMER_BODY_PART,
                RECV_CMD_CONSUMER_REMOVE_MESSAGE,
             
                RECV_CMD_PRODUCER_BODY_PART,
            };
             
            enum Flag {
                FLAG_LOCK_RECV = 1,
                FLAG_CLOSE = 2,
                FLAG_OBSERVE_INACTIVE = 4,
                FLAG_CRASHED_MESSAGE = 8,
                FLAG_SEND = 16,
                FLAG_RECV_RAW = 32,
                FLAG_SEND_RAW = 64,
                FLAG_ERROR = 128,
            };
             
            struct Session {
                TypeRecv typeRecv;
                unsigned int flag;
             
                std::unique_ptr<char[]> authData;
                std::unique_ptr<Protocol::Packet> packet;
                unsigned short int offsetChannel;
                unsigned short int offsetLogin;
             
                unsigned int ip;
                unsigned int lastTS;
             
                unsigned int msgID;
                unsigned int lengthMessage;
                unsigned int sendLengthMessage;
            };

            std::map<unsigned int, std::unique_ptr<Session>> _sessions;
            Access *_access = nullptr;
            q::Manager *_q = nullptr;
            Changes *_changes = nullptr;

            void _setSendFlag( Session *sess );
            void _unsetSendFlag( Session *sess );

            void _sendOk( unsigned int fd, Session *sess );
            void _sendError( unsigned int fd, Session *sess, const char *err );
            void _sendVersion( unsigned int fd, Session *sess );
            void _continueSend( unsigned int fd, Session *sess );

            void _checkCmd( unsigned int fd, Session *sess );
            void _checkSecureCmd( unsigned int fd, Session *sess );
            void _checkGetVersionCmd( unsigned int fd, Session *sess );
            void _checkAuthCmd( unsigned int fd, Session *sess );
            void _checkAuthGroupCmd( unsigned int fd, Session *sess );
            void _checkAuthConsumerCmd( unsigned int fd, Session *sess );
            void _checkAuthProducerCmd( unsigned int fd, Session *sess );
            void _checkUpdateMyGroupPasswordCmd( unsigned int fd, Session *sess );
            void _checkUpdateMyConsumerPasswordCmd( unsigned int fd, Session *sess );
            void _checkUpdateConsumerPasswordCmd( unsigned int fd, Session *sess );
            void _checkUpdateMyProducerPasswordCmd( unsigned int fd, Session *sess );
            void _checkUpdateProducerPasswordCmd( unsigned int fd, Session *sess );
            void _checkGroupCmd( unsigned int fd, Session *sess );
            void _checkConsumerCmd( unsigned int fd, Session *sess );
            void _checkProducerCmd( unsigned int fd, Session *sess );

        public:
            void connect( unsigned int fd, unsigned int ip );
            void recv( unsigned int fd );
            void send( unsigned int fd );
            void disconnect( unsigned int fd );
            void iteration();
    };

    void ServerController::_setSendFlag( Session *sess ) {
        sess->flag |= FLAG_LOCK_RECV | FLAG_SEND;
        sess->typeRecv = RECV_GET_VERSION;
    }

    void ServerController::_unsetSendFlag( Session *sess ) {
        sess->flag ^= FLAG_SEND;
        sess->flag ^= FLAG_LOCK_RECV;
    }

    void ServerController::_sendOk( unsigned int fd, Session *sess ) {
        _setSendFlag( sess );

        if( Protocol::sendOk( fd, sess->packet.get() ) ) {
            _unsetSendFlag( sess );
        }
    }

    void ServerController::_sendError( unsigned int fd, Session *sess, const char *err ) {
        _setSendFlag( sess );

        if( Protocol::sendError( fd, sess->packet.get(), err ) ) {
            _unsetSendFlag( sess );
        }

        sess->flag &= FLAG_ERROR;
    }
    
    void ServerController::_sendVersion( unsigned int fd, Session *sess ) {
        _setSendFlag( sess );

        if( Protocol::sendVersion( fd, sess->packet.get() ) ) {
            _unsetSendFlag( sess );
        }
    }

    void ServerController::_continueSend( unsigned int fd, Session *sess ) {
        if( Protocol::continueSend( fd, sess->packet.get() ) ) {
            _unsetSendFlag( sess );
        }
    }

    void ServerController::_checkSecureCmd( unsigned int fd, Session *sess ) {
        if( Protocol::isCheckNoSecure( sess->packet.get() ) ) {
            _sendOk( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkGetVersionCmd( unsigned int fd, Session *sess ) {
        if( Protocol::isCheckVersion( sess->packet.get() ) ) {
            _sendVersion( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkAuthGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto password = Protocol::getPassword( packet );

        _access->authGroup( group, password, fd );

        _sendOk( fd, sess );
    }

    void ServerController::_checkAuthConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authConsumer( group, channel, login, password, fd );
        _q->joinConsumer( group, channel, fd );

        _sendOk( fd, sess );
    }

    void ServerController::_checkAuthProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authProducer( group, channel, login, password, fd );
        _q->joinProducer( group, channel, fd );

        _sendOk( fd, sess );
    }

    void ServerController::_checkAuthCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( Protocol::isAuthGroup( packet ) ) {
            _checkAuthGroupCmd( fd, sess );
        } else if( Protocol::isAuthConsumer( packet ) ) {
            _checkAuthConsumerCmd( fd, sess );
        } else if( Protocol::isAuthProducer( packet ) ) {
            _checkAuthProducerCmd( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkUpdateMyGroupPasswordCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto curPassword = Protocol::getPassword( packet );
        auto newPassword = Protocol::getNewPassword( packet );

        auto group = sess->authData.get();

        _access->checkUpdateMyGroupPassword( group, curPassword, fd );
        auto change = _changes->updateGroupPassword( group, newPassword );

        _changes->push(
            std::move( change ),
            sess->ip,
            util::types::Initiator::I_GROUP,
            group
        );

        _sendOk( fd, sess );
    }

    void ServerController::_checkUpdateMyConsumerPasswordCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto curPassword = Protocol::getPassword( packet );
        auto newPassword = Protocol::getNewPassword( packet );

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkUpdateMyConsumerPassword( group, channel, login, curPassword, fd );
        auto change = _changes->updateConsumerPassword(
            group,
            channel,
            login,
            newPassword
        );

        _changes->push(
            std::move( change ),
            sess->ip,
            util::types::Initiator::I_CONSUMER,
            group, channel, login
        );

        _sendOk( fd, sess );
    }

    void ServerController::_checkUpdateConsumerPasswordCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto newPassword = Protocol::getPassword( packet );

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );

        _access->checkUpdateConsumerPassword( group, fd, channel, login );
        auto change = _changes->updateConsumerPassword(
            group,
            channel,
            login,
            newPassword
        );

        _changes->push(
            std::move( change ),
            sess->ip,
            util::types::Initiator::I_GROUP,
            group
        );

        _sendOk( fd, sess );
    }

    void ServerController::_checkUpdateMyProducerPasswordCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto curPassword = Protocol::getPassword( packet );
        auto newPassword = Protocol::getNewPassword( packet );

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkUpdateMyProducerPassword( group, channel, login, curPassword, fd );
        auto change = _changes->updateProducerPassword(
            group,
            channel,
            login,
            newPassword
        );

        _changes->push(
            std::move( change ),
            sess->ip,
            util::types::Initiator::I_PRODUCER,
            group, channel, login
        );

        _sendOk( fd, sess );
    }

    void ServerController::_checkUpdateProducerPasswordCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto newPassword = Protocol::getPassword( packet );

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );

        _access->checkUpdateProducerPassword( group, fd, channel, login );
        auto change = _changes->updateProducerPassword(
            group,
            channel,
            login,
            newPassword
        );

        _changes->push(
            std::move( change ),
            sess->ip,
            util::types::Initiator::I_GROUP,
            group
        );

        _sendOk( fd, sess );
    }

    void ServerController::_checkGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( Protocol::isUpdatePassword( packet ) ) {
            _checkUpdateMyGroupPasswordCmd( fd, sess );
        } else if( Protocol::isGetChannels( packet ) ) {
        } else if( Protocol::isGetChannelLimitMessages( packet ) ) {
        } else if( Protocol::isGetConsumers( packet ) ) {
        } else if( Protocol::isGetProducers( packet ) ) {
        } else if( Protocol::isAddChannel( packet ) ) {
        } else if( Protocol::isUpdateChannelLimitMessages( packet ) ) {
        } else if( Protocol::isRemoveChannel( packet ) ) {
        } else if( Protocol::isAddConsumer( packet ) ) {
        } else if( Protocol::isUpdateConsumerPassword( packet ) ) {
            _checkUpdateConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveConsumer( packet ) ) {
        } else if( Protocol::isAddProducer( packet ) ) {
        } else if( Protocol::isUpdateProducerPassword( packet ) ) {
            _checkUpdateProducerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveProducer( packet ) ) {
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( Protocol::isUpdatePassword( packet ) ) {
            _checkUpdateMyConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isPopMessage( packet ) ) {
        } else if( Protocol::isRemoveMessageByUUID( packet ) ) {
        } else if( Protocol::isOk( packet ) ) {
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( Protocol::isUpdatePassword( packet ) ) {
            _checkUpdateMyProducerPasswordCmd( fd, sess );
        } else if( Protocol::isPushMessage( packet ) ) {
            sess->typeRecv = RECV_CMD_PRODUCER_BODY_PART;
        } else if( Protocol::isPushPublicMessage( packet ) ) {
            sess->typeRecv = RECV_CMD_PRODUCER_BODY_PART;
        } else if( Protocol::isPushReplicaMessage( packet ) ) {
            sess->typeRecv = RECV_CMD_PRODUCER_BODY_PART;
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_checkCmd( unsigned int fd, Session *sess ) {
        switch( sess->typeRecv ) {
            case RECV_CHECK_SECURE:
                _checkSecureCmd( fd, sess );
                break;
            case RECV_GET_VERSION:
                _checkGetVersionCmd( fd, sess );
                break;
            case RECV_AUTH:
                _checkAuthCmd( fd, sess );
                break;
            case RECV_CMD_GROUP:
                _checkGroupCmd( fd, sess );
                break;
            case RECV_CMD_CONSUMER:
                _checkConsumerCmd( fd, sess );
                break;
            case RECV_CMD_PRODUCER:
                _checkProducerCmd( fd, sess );
                break;
            case RECV_CMD_CONSUMER_BODY_PART:
                break;
            case RECV_CMD_CONSUMER_REMOVE_MESSAGE:
                break;
            case RECV_CMD_PRODUCER_BODY_PART:
                break;
        }
    }

    void ServerController::connect( unsigned int fd, unsigned int ip ) {
        auto sess = std::make_unique<Session>();
        sess->typeRecv = RECV_CHECK_SECURE;
        sess->packet = std::make_unique<Protocol::Packet>();
        _sessions[fd] = std::move( sess );
    }

    void ServerController::recv( unsigned int fd ) {
        auto sess = _sessions[fd].get();
        auto packet = sess->packet.get();

        try {
            if( FLAG_LOCK_RECV & sess->flag && !( FLAG_LOCK_RECV & sess->flag ) ) {
                sess->flag |= FLAG_CLOSE;
                throw util::Error::WRONG_CMD;
            }

            if( RECV_CMD_PRODUCER_BODY_PART == sess->typeRecv ) {
                return;
            }

            Protocol::recv( fd, packet );

            if( !Protocol::isReceived( packet ) ) {
                return;
            }

            _checkCmd( fd, sess );

            if( FLAG_SEND & sess->flag ) {
                return;
            }

            if( RECV_CHECK_SECURE == sess->typeRecv ) {
                sess->typeRecv = RECV_GET_VERSION;
            } else if( RECV_GET_VERSION == sess->typeRecv ) {
                sess->typeRecv = RECV_AUTH;
            } else if( RECV_AUTH == sess->typeRecv ) {
            }

        } catch( util::Error::Err err ) {
            _sendError( fd, sess, util::Error::getDescription( err ) );
        } catch( ... ) {
            _sendError( fd, sess, util::Error::getDescription( util::Error::UNKNOWN ) );
        }

    }

    void ServerController::send( unsigned int fd ) {
        auto sess = _sessions[fd].get();

        if( FLAG_ERROR & sess->flag ) {
            _sessions.erase( fd );
            ::close( fd );
            return;
        }

        if( FLAG_SEND_RAW & sess->flag ) {
            return;
        }

        if( !( FLAG_SEND & sess->flag ) ) {
            return;
        }

        try {
            _continueSend( fd, sess );
        } catch( ... ) {
            _sessions.erase( fd );
            ::close( fd );
        }
    }

    void ServerController::disconnect( unsigned int fd ) {
        _sessions.erase( fd );
    }

    void ServerController::iteration() {
    }
}

#endif
 
