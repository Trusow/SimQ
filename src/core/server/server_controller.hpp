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
            enum FSM {
                FSM_COMMON_RECV_CMD_CHECK_SECURE,
                FSM_COMMON_SEND_CMD_SECURE_OK,

                FSM_COMMON_RECV_CMD_GET_VERSION,
                FSM_COMMON_SEND_CMD_VERSION,

                FSM_COMMON_RECV_CMD_AUTH,
                FSM_COMMON_SEND_CMD_OK_GROUP,
                FSM_COMMON_SEND_CMD_OK_CONSUMER,
                FSM_COMMON_SEND_CMD_OK_PRODUCER,

                FSM_GROUP_RECV,
                FSM_GROUP_SEND,
                FSM_GROUP_SEND_ERROR,

                FSM_CONSUMER_RECV,
                FSM_CONSUMER_SEND,
                FSM_CONSUMER_SEND_ERROR,
                FSM_CONSUMER_SEND_PART_MESSAGE,
                FSM_CONSUMER_SEND_PART_MESSAGE_NULL,
                FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_OK,
                FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_ERROR,
                FSM_CONSUMER_RECV_REMOVE_MESSAGE,

                FSM_PRODUCER_RECV,
                FSM_PRODUCER_SEND,
                FSM_PRODUCER_SEND_ERROR,
                FSM_PRODUCER_RECV_PART_MESSAGE,
                FSM_PRODUCER_RECV_PART_MESSAGE_NULL,
                FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_OK,
                FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_ERROR,

                FSM_CLOSE_COMMON,
                FSM_CLOSE_GROUP,
                FSM_CLOSE_CONSUMER,
                FSM_CLOSE_PRODUCER,
            };

            struct Session {
                FSM fsm;
             
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

            void _sendVersion( unsigned int fd, Session *sess );
            void _sendSecureOk( unsigned int fd, Session *sess );
            void _sendError(
                unsigned int fd,
                Session *sess,
                const char *err = nullptr,
                bool isClose = true
            );
            void _sendAuthGroupOk( unsigned int fd, Session *sess );
            void _sendAuthConsumerOk( unsigned int fd, Session *sess );
            void _sendAuthProducerOk( unsigned int fd, Session *sess );
            void _sendRemoveMessageOk( unsigned int fd, Session *sess );
            void _sendGroup( unsigned int fd, Session *sess );
            void _sendConsumer( unsigned int fd, Session *sess );
            void _sendProducer( unsigned int fd, Session *sess );

            bool _recvToPacket( unsigned int fd, Protocol::Packet *packet );
            FSM _getFSMCloseFlag( FSM fsm );
            bool _isCloseFSMFlag( FSM fsm );

            void _recvSecure( unsigned int fd, Session *sess );
            void _recvVersion( unsigned int fd, Session *sess );
            void _recvAuth( unsigned int fd, Session *sess );
            void _authGroupCmd( unsigned int fd, Session *sess );
            void _authConsumerCmd( unsigned int fd, Session *sess );
            void _authProducerCmd( unsigned int fd, Session *sess );
            void _updateMyGroupPasswordCmd( unsigned int fd, Session *sess );
            void _updateMyConsumerPasswordCmd( unsigned int fd, Session *sess );
            void _updateConsumerPasswordCmd( unsigned int fd, Session *sess );
            void _updateMyProducerPasswordCmd( unsigned int fd, Session *sess );
            void _updateProducerPasswordCmd( unsigned int fd, Session *sess );
            void _recvGroupCmd( unsigned int fd, Session *sess );
            void _recvConsumerCmd( unsigned int fd, Session *sess );
            void _recvConsumerRemoveMessage( unsigned int fd, Session *sess );
            void _recvProducerCmd( unsigned int fd, Session *sess );
            void _recvProducerPartMessage( unsigned int fd, Session *sess );
            void _recvProducerPartMessageNull( unsigned int fd, Session *sess );

        public:
            void connect( unsigned int fd, unsigned int ip );
            void recv( unsigned int fd );
            void send( unsigned int fd );
            void disconnect( unsigned int fd );
            void iteration();
    };

    ServerController::FSM ServerController::_getFSMCloseFlag( FSM fsm ) {
        switch( fsm ) {
            case FSM_COMMON_RECV_CMD_CHECK_SECURE:
            case FSM_COMMON_SEND_CMD_SECURE_OK:
            case FSM_COMMON_RECV_CMD_GET_VERSION:
            case FSM_COMMON_SEND_CMD_VERSION:
            case FSM_COMMON_RECV_CMD_AUTH:
            case FSM_COMMON_SEND_CMD_OK_GROUP:
            case FSM_COMMON_SEND_CMD_OK_CONSUMER:
            case FSM_COMMON_SEND_CMD_OK_PRODUCER:
                return FSM_CLOSE_COMMON;
            case FSM_GROUP_RECV:
            case FSM_GROUP_SEND:
            case FSM_GROUP_SEND_ERROR:
                return FSM_CLOSE_GROUP;
            case FSM_CONSUMER_RECV:
            case FSM_CONSUMER_SEND:
            case FSM_CONSUMER_SEND_PART_MESSAGE:
            case FSM_CONSUMER_SEND_PART_MESSAGE_NULL:
            case FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_OK:
            case FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_ERROR:
            case FSM_CONSUMER_RECV_REMOVE_MESSAGE:
                return FSM_CLOSE_CONSUMER;
            case FSM_PRODUCER_RECV:
            case FSM_PRODUCER_SEND:
            case FSM_PRODUCER_RECV_PART_MESSAGE:
            case FSM_PRODUCER_RECV_PART_MESSAGE_NULL:
            case FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_OK:
            case FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_ERROR:
                return FSM_CLOSE_PRODUCER;
            case FSM_CLOSE_COMMON:
            case FSM_CLOSE_GROUP:
            case FSM_CLOSE_CONSUMER:
            case FSM_CLOSE_PRODUCER:
                return fsm;
            default:
                return fsm;
        }
    }

    bool ServerController::_isCloseFSMFlag( FSM fsm ) {
        switch( fsm ) {
            case FSM_CLOSE_COMMON:
            case FSM_CLOSE_GROUP:
            case FSM_CLOSE_CONSUMER:
            case FSM_CLOSE_PRODUCER:
                return true;
            default:
                return false;
        }
    }

    bool ServerController::_recvToPacket( unsigned int fd, Protocol::Packet *packet ) {
        Protocol::recv( fd, packet );

        return Protocol::isReceived( packet );
    }

    void ServerController::_sendVersion( unsigned int fd, Session *sess ) {
        if( sess->fsm != FSM_COMMON_SEND_CMD_VERSION ) {
            if( Protocol::sendVersion( fd, sess->packet.get() ) ) {
                sess->fsm = FSM_COMMON_RECV_CMD_AUTH;
                Protocol::reset( sess->packet.get() );
            } else {
                sess->fsm = FSM_COMMON_SEND_CMD_VERSION;
            }
        } else {
            if( Protocol::continueSend( fd, sess->packet.get() ) ) {
                sess->fsm = FSM_COMMON_RECV_CMD_AUTH;
                Protocol::reset( sess->packet.get() );
            }
        }
    }

    void ServerController::_sendSecureOk( unsigned int fd, Session *sess ) {
        if( sess->fsm != FSM_COMMON_SEND_CMD_SECURE_OK ) {
            if( Protocol::sendOk( fd, sess->packet.get() ) ) {
                sess->fsm = FSM_COMMON_RECV_CMD_GET_VERSION;
                Protocol::reset( sess->packet.get() );
            } else {
                sess->fsm = FSM_COMMON_SEND_CMD_SECURE_OK;
            }
        } else {
            if( Protocol::continueSend( fd, sess->packet.get() ) ) {
                sess->fsm = FSM_COMMON_RECV_CMD_GET_VERSION;
                Protocol::reset( sess->packet.get() );
            }
        }
    }

    void ServerController::_recvSecure( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isCheckNoSecure( sess->packet.get() ) ) {
            _sendSecureOk( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvVersion( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isGetVersion( sess->packet.get() ) ) {
            _sendVersion( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_authGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto password = Protocol::getPassword( packet );

        _access->authGroup( group, password, fd );

        _sendAuthGroupOk( fd, sess );
    }

    void ServerController::_authConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authConsumer( group, channel, login, password, fd );
        _q->joinConsumer( group, channel, fd );

        _sendAuthConsumerOk( fd, sess );
    }

    void ServerController::_authProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authProducer( group, channel, login, password, fd );
        _q->joinProducer( group, channel, fd );

        _sendAuthProducerOk( fd, sess );
    }

    void ServerController::_recvAuth( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isAuthGroup( packet ) ) {
            _authGroupCmd( fd, sess );
        } else if( Protocol::isAuthConsumer( packet ) ) {
            _authConsumerCmd( fd, sess );
        } else if( Protocol::isAuthProducer( packet ) ) {
            _authProducerCmd( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_updateMyGroupPasswordCmd( unsigned int fd, Session *sess ) {
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

    }

    void ServerController::_updateMyConsumerPasswordCmd( unsigned int fd, Session *sess ) {
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

    }

    void ServerController::_updateConsumerPasswordCmd( unsigned int fd, Session *sess ) {
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

    }

    void ServerController::_updateMyProducerPasswordCmd( unsigned int fd, Session *sess ) {
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

    }

    void ServerController::_updateProducerPasswordCmd( unsigned int fd, Session *sess ) {
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

    }

    void ServerController::_recvGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        sess->fsm = FSM_GROUP_SEND_ERROR;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyGroupPasswordCmd( fd, sess );
        } else if( Protocol::isGetChannels( packet ) ) {
        } else if( Protocol::isGetChannelLimitMessages( packet ) ) {
        } else if( Protocol::isGetConsumers( packet ) ) {
        } else if( Protocol::isGetProducers( packet ) ) {
        } else if( Protocol::isAddChannel( packet ) ) {
        } else if( Protocol::isUpdateChannelLimitMessages( packet ) ) {
        } else if( Protocol::isRemoveChannel( packet ) ) {
        } else if( Protocol::isAddConsumer( packet ) ) {
        } else if( Protocol::isUpdateConsumerPassword( packet ) ) {
            _updateConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveConsumer( packet ) ) {
        } else if( Protocol::isAddProducer( packet ) ) {
        } else if( Protocol::isUpdateProducerPassword( packet ) ) {
            _updateProducerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveProducer( packet ) ) {
        } else {
            sess->fsm = FSM_GROUP_RECV;
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        sess->fsm = FSM_CONSUMER_SEND_ERROR;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isPopMessage( packet ) ) {
        } else if( Protocol::isRemoveMessageByUUID( packet ) ) {
        } else if( Protocol::isOk( packet ) ) {
        } else {
            sess->fsm = FSM_CONSUMER_RECV;
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvConsumerRemoveMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        sess->fsm = FSM_CONSUMER_SEND_ERROR;

        if( Protocol::isRemoveMessage( packet ) ) {
            _sendRemoveMessageOk( fd, sess );
        } else {
            sess->fsm = FSM_CONSUMER_RECV;
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        sess->fsm = FSM_PRODUCER_SEND_ERROR;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyProducerPasswordCmd( fd, sess );
        } else if( Protocol::isPushMessage( packet ) ) {
        } else if( Protocol::isPushPublicMessage( packet ) ) {
        } else if( Protocol::isPushReplicaMessage( packet ) ) {
        } else {
            sess->fsm = FSM_PRODUCER_RECV;
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvProducerPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        //
    }

    void ServerController::_recvProducerPartMessageNull( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
    }

    void ServerController::_sendError(
        unsigned int fd,
        Session *sess,
        const char *err,
        bool isClose
    ) {
        if( err != nullptr ) {
            if( isClose ) {
                sess->fsm = _getFSMCloseFlag( sess->fsm );
            }

            try {
                Protocol::sendError(
                    fd,
                    sess->packet.get(),
                    err
                );
            } catch( ... ) {
                _sessions.erase( fd );
                ::close( fd );
            }

            return;
        }

        if( Protocol::continueSend( fd, sess->packet.get() ) ) {
            if( isClose ) {
                _sessions.erase( fd );
                ::close( fd );
            }
        }
    }

    void ServerController::_sendAuthGroupOk( unsigned int fd, Session *sess ) {
        if( Protocol::sendOk( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_GROUP_RECV;
        }
    }

    void ServerController::_sendAuthConsumerOk( unsigned int fd, Session *sess ) {
        if( Protocol::sendOk( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_CONSUMER_RECV;
        }
    }

    void ServerController::_sendAuthProducerOk( unsigned int fd, Session *sess ) {
        if( Protocol::sendOk( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_PRODUCER_RECV;
        }
    }

    void ServerController::_sendRemoveMessageOk( unsigned int fd, Session *sess ) {
        if( Protocol::sendOk( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_CONSUMER_RECV;
        }
    }

    void ServerController::_sendGroup( unsigned int fd, Session *sess ) {
        if( Protocol::continueSend( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_GROUP_RECV;
        }
    }

    void ServerController::_sendConsumer( unsigned int fd, Session *sess ) {
        if( Protocol::continueSend( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_CONSUMER_RECV;
        }
    }

    void ServerController::_sendProducer( unsigned int fd, Session *sess ) {
        if( Protocol::continueSend( fd, sess->packet.get() ) ) {
            sess->fsm = FSM_PRODUCER_RECV;
        }
    }

    void ServerController::connect( unsigned int fd, unsigned int ip ) {
        auto sess = std::make_unique<Session>();
        sess->fsm = FSM_COMMON_RECV_CMD_CHECK_SECURE;
        sess->packet = std::make_unique<Protocol::Packet>();
        _sessions[fd] = std::move( sess );
    }

    void ServerController::recv( unsigned int fd ) {
        auto sess = _sessions[fd].get();

        try {
            switch( sess->fsm ) {
                case FSM_COMMON_RECV_CMD_CHECK_SECURE:
                    _recvSecure( fd, sess );
                    break;
                case FSM_COMMON_RECV_CMD_GET_VERSION:
                    _recvVersion( fd, sess );
                    break;
                case FSM_COMMON_RECV_CMD_AUTH:
                    _recvAuth( fd, sess );
                    break;
                case FSM_GROUP_RECV:
                    _recvGroupCmd( fd, sess );
                    break;
                case FSM_CONSUMER_RECV:
                    _recvConsumerCmd( fd, sess );
                    break;
                case FSM_CONSUMER_RECV_REMOVE_MESSAGE:
                    _recvConsumerRemoveMessage( fd, sess );
                    break;
                case FSM_PRODUCER_RECV:
                    _recvProducerCmd( fd, sess );
                    break;
                case FSM_PRODUCER_RECV_PART_MESSAGE:
                    _recvProducerPartMessage( fd, sess );
                    break;
                case FSM_PRODUCER_RECV_PART_MESSAGE_NULL:
                    _recvProducerPartMessageNull( fd, sess );
                    break;
                case FSM_CLOSE_COMMON:
                case FSM_CLOSE_GROUP:
                case FSM_CLOSE_CONSUMER:
                case FSM_CLOSE_PRODUCER:
                    break;
                default:
                    sess->fsm = _getFSMCloseFlag( sess->fsm );
                    throw util::Error::Err::WRONG_CMD;
            }
        } catch( util::Error::Err err ) {
            sess->fsm = _getFSMCloseFlag( sess->fsm );
            _sendError(
                fd,
                sess,
                util::Error::getDescription( err ),
                _isCloseFSMFlag( sess->fsm )
            );
        } catch( ... ) {
            _sendError(
                fd,
                sess,
                util::Error::getDescription( util::Error::Err::UNKNOWN ),
                true
            );
        }

    }

    void ServerController::send( unsigned int fd ) {
        auto sess = _sessions[fd].get();

        try {
            switch( sess->fsm ) {
                case FSM_COMMON_SEND_CMD_SECURE_OK:
                    _sendSecureOk( fd, sess );
                    break;
                case FSM_COMMON_SEND_CMD_VERSION:
                    _sendVersion( fd, sess );
                    break;
                case FSM_COMMON_SEND_CMD_OK_GROUP:
                    _sendAuthGroupOk( fd, sess );
                    break;
                case FSM_COMMON_SEND_CMD_OK_CONSUMER:
                    _sendAuthConsumerOk( fd, sess );
                    break;
                case FSM_COMMON_SEND_CMD_OK_PRODUCER:
                    _sendAuthProducerOk( fd, sess );
                    break;
                case FSM_GROUP_SEND:
                    _sendGroup( fd, sess );
                    break;
                case FSM_GROUP_SEND_ERROR:
                    _sendError( fd, sess, nullptr, false );
                    break;
                case FSM_CONSUMER_SEND:
                    _sendConsumer( fd, sess );
                    break;
                case FSM_CONSUMER_SEND_ERROR:
                    _sendError( fd, sess, nullptr, false );
                    break;
                case FSM_CONSUMER_SEND_PART_MESSAGE:
                    // _sendConsumerPartMessage( fd, sess );
                    break;
                case FSM_CONSUMER_SEND_PART_MESSAGE_NULL:
                    // _sendSendConsumerPartMessageNull( fd, sess );
                    break;
                case FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_OK:
                    // _sendSendConsumerPartMessageConfirmOk( fd, sess );
                    break;
                case FSM_CONSUMER_SEND_PART_MESSAGE_CONFIRM_ERROR:
                    // _sendSendConsumerPartMessageConfirmError( fd, sess );
                    break;
                case FSM_PRODUCER_SEND:
                    _sendProducer( fd, sess );
                    break;
                case FSM_PRODUCER_SEND_ERROR:
                    _sendError( fd, sess, nullptr, false );
                    break;
                case FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_OK:
                    // _sendSendProducerPartMessageConfirmOk( fd, sess );
                    break;
                case FSM_PRODUCER_SEND_PART_MESSAGE_CONFIRM_ERROR:
                    // _sendSendProducerPartMessageConfirmError( fd, sess );
                    break;
                case FSM_CLOSE_COMMON:
                case FSM_CLOSE_GROUP:
                case FSM_CLOSE_CONSUMER:
                case FSM_CLOSE_PRODUCER:
                    _sendError( fd, sess, nullptr, true );
                    break;
                default:
                    return;
            }
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
 
