#ifndef SIMQ_CORE_SERVER_SERVER_CONTROLLER
#define SIMQ_CORE_SERVER_SERVER_CONTROLLER

#include <unordered_map>
#include <memory>
#include <vector>
#include <unistd.h>
#include <string.h>
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
                FSM_COMMON_SEND_CONFIRM_SECURE,
                FSM_COMMON_RECV_CMD_GET_VERSION,
                FSM_COMMON_SEND_VERSION,
                FSM_COMMON_RECV_CMD_AUTH,
                FSM_COMMON_SEND_CONFIRM_AUTH_GROUP,
                FSM_COMMON_SEND_CONFIRM_AUTH_CONSUMER,
                FSM_COMMON_SEND_CONFIRM_AUTH_PRODUCER,
                FSM_COMMON_SEND_ERROR_WITH_CLOSE,

                FSM_COMMON_CLOSE,
                

                 
                FSM_GROUP_RECV_CMD,
                FSM_GROUP_SEND,
                FSM_GROUP_SEND_ERROR,
                FSM_GROUP_SEND_ERROR_WITH_CLOSE,

                FSM_GROUP_CLOSE,


                 
                FSM_CONSUMER_RECV_CMD,
                FSM_CONSUMER_SEND,
                FSM_CONSUMER_SEND_ERROR,
                FSM_CONSUMER_SEND_ERROR_WITH_CLOSE,
                 
                FSM_CONSUMER_RECV_CMD_PART_MESSAGE,
                FSM_CONSUMER_SEND_PART_MESSAGE,
                FSM_CONSUMER_SEND_PART_MESSAGE_NULL,
                FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE,
                FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE,

                FSM_CONSUMER_CLOSE,
                 

                FSM_PRODUCER_RECV_CMD,
                FSM_PRODUCER_SEND,
                FSM_PRODUCER_SEND_ERROR,
                FSM_PRODUCER_SEND_ERROR_WITH_CLOSE,
                 
                FSM_PRODUCER_RECV_PART_MESSAGE,
                FSM_PRODUCER_RECV_PART_MESSAGE_NULL,
                FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE,
                 
                FSM_PRODUCER_CLOSE,
            };

            enum Type {
                TYPE_COMMON,
                TYPE_GROUP,
                TYPE_CONSUMER,
                TYPE_PRODUCER,
            };
            
            Type getTypeByFSM( FSM fsm );

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

            FSM _getFSMAfterSendPartMessage( Session *sess );
            FSM _getFSMAfterRecvPartMessage( Session *sess );
            FSM _getFSMAfterSendConfirmToConsumer( Session *sess );
            FSM _getFSMAfterSendConfirmToProducer( Session *sess );
            FSM _getFSMByError( FSM fsm, util::Error::Err err );

            void _close( unsigned int fd, Session *sess );

            bool _recvToPacket( unsigned int fd, Protocol::Packet *packet );

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
            void _recvConsumerCmdPartMessage( unsigned int fd, Session *sess );
            void _recvConsumerRemoveMessage( unsigned int fd, Session *sess );
            void _recvProducerCmd( unsigned int fd, Session *sess );
            void _recvFromProducerPartMessage( unsigned int fd, Session *sess );
            void _recvFromProducerPartMessageNull( unsigned int fd, Session *sess );
            void _sendToConsumerPartMessage( unsigned int fd, Session *sess );
            void _sendToConsumerPartMessageNull( unsigned int fd, Session *sess );
            void _send( unsigned int fd, Session *sess );

        public:
            void connect( unsigned int fd, unsigned int ip );
            void recv( unsigned int fd );
            void send( unsigned int fd );
            void disconnect( unsigned int fd );
            void iteration();
    };

    ServerController::FSM ServerController::_getFSMByError( FSM fsm, util::Error::Err err ) {
        auto type = getTypeByFSM( fsm );

        switch( type ) {
            case TYPE_GROUP:
                switch( err ) {
                    case util::Error::SOCKET:
                        return FSM_GROUP_CLOSE;
                    case util::Error::WRONG_PASSWORD:
                    case util::Error::WRONG_PARAM:
                    case util::Error::WRONG_GROUP:
                    case util::Error::WRONG_CHANNEL:
                    case util::Error::WRONG_LOGIN:
                    case util::Error::WRONG_MESSAGE_SIZE:
                    case util::Error::WRONG_SETTINGS:
                    case util::Error::WRONG_CHANNEL_LIMIT_MESSAGES:
                    case util::Error::WRONG_CONSUMER:
                    case util::Error::WRONG_PRODUCER:
                        return FSM_GROUP_SEND_ERROR;
                    default:
                        return FSM_GROUP_SEND_ERROR_WITH_CLOSE;
                }
            case TYPE_CONSUMER:
                switch( err ) {
                    case util::Error::SOCKET:
                        return FSM_CONSUMER_CLOSE;
                    case util::Error::WRONG_PASSWORD:
                        return FSM_CONSUMER_SEND_ERROR;
                    default:
                        return FSM_CONSUMER_SEND_ERROR_WITH_CLOSE;
                }
            case TYPE_PRODUCER:
                switch( err ) {
                    case util::Error::SOCKET:
                        return FSM_PRODUCER_CLOSE;
                    case util::Error::WRONG_PASSWORD:
                    case util::Error::EXCEED_LIMIT:
                        return FSM_PRODUCER_SEND_ERROR;
                    default:
                        return FSM_PRODUCER_SEND_ERROR_WITH_CLOSE;
                }
            default:
                switch( err ) {
                    case util::Error::SOCKET:
                        return FSM_COMMON_CLOSE;
                    default:
                        return FSM_COMMON_SEND_ERROR_WITH_CLOSE;
                }
        }
    }

    void ServerController::_close( unsigned int fd, Session *sess ) {
        auto type = getTypeByFSM( sess->fsm );

        switch( type ) {
            case TYPE_GROUP:
                _access->logoutGroup( sess->authData.get(), fd );
                break;
            case TYPE_CONSUMER:
                _access->logoutConsumer(
                    sess->authData.get(),
                    &sess->authData.get()[sess->offsetChannel],
                    &sess->authData.get()[sess->offsetLogin],
                    fd
                );
                _q->leaveConsumer(
                    sess->authData.get(),
                    &sess->authData.get()[sess->offsetChannel],
                    fd
                );
                break;
            case TYPE_PRODUCER:
                _access->logoutProducer(
                    sess->authData.get(),
                    &sess->authData.get()[sess->offsetChannel],
                    &sess->authData.get()[sess->offsetLogin],
                    fd
                );
                _q->leaveProducer(
                    sess->authData.get(),
                    &sess->authData.get()[sess->offsetChannel],
                    fd
                );
                break;
        }

        _sessions.erase( fd );
    }

    ServerController::Type ServerController::getTypeByFSM( FSM fsm ) {
        switch( fsm ) {
            case FSM_GROUP_RECV_CMD:
            case FSM_GROUP_SEND:
            case FSM_GROUP_SEND_ERROR:
            case FSM_GROUP_SEND_ERROR_WITH_CLOSE:
            case FSM_GROUP_CLOSE:
                return TYPE_GROUP;

            case FSM_CONSUMER_RECV_CMD:
            case FSM_CONSUMER_SEND:
            case FSM_CONSUMER_SEND_ERROR:
            case FSM_CONSUMER_SEND_ERROR_WITH_CLOSE:
            case FSM_CONSUMER_RECV_CMD_PART_MESSAGE:
            case FSM_CONSUMER_SEND_PART_MESSAGE:
            case FSM_CONSUMER_SEND_PART_MESSAGE_NULL:
            case FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE:
            case FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE:
            case FSM_CONSUMER_CLOSE:
                return TYPE_CONSUMER;

            case FSM_PRODUCER_RECV_CMD:
            case FSM_PRODUCER_SEND:
            case FSM_PRODUCER_SEND_ERROR:
            case FSM_PRODUCER_SEND_ERROR_WITH_CLOSE:
            case FSM_PRODUCER_RECV_PART_MESSAGE:
            case FSM_PRODUCER_RECV_PART_MESSAGE_NULL:
            case FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE:
            case FSM_PRODUCER_CLOSE:
                return TYPE_PRODUCER;

            default:
                return TYPE_COMMON;
        }
    }

    bool ServerController::_recvToPacket( unsigned int fd, Protocol::Packet *packet ) {
        Protocol::recv( fd, packet );

        return Protocol::isReceived( packet );
    }

    ServerController::FSM ServerController::_getFSMAfterRecvPartMessage( Session *sess ) {
        switch( sess->fsm ) {
            case FSM_PRODUCER_RECV_PART_MESSAGE:
                return FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE;
            default:
                return FSM_PRODUCER_SEND_ERROR_WITH_CLOSE;
        }
    }

    ServerController::FSM ServerController::_getFSMAfterSendPartMessage( Session *sess ) {
        switch( sess->fsm ) {
            case FSM_CONSUMER_SEND_PART_MESSAGE:
                return FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE;
            default:
                return FSM_CONSUMER_SEND_ERROR_WITH_CLOSE;
        }
    }

    ServerController::FSM ServerController::_getFSMAfterSendConfirmToConsumer( Session *sess ) {
        //FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE | FSM_CONSUMER_RECV_CMD_PART_MESSAGE
        return FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE;
    }

    ServerController::FSM ServerController::_getFSMAfterSendConfirmToProducer( Session *sess ) {
        //FSM_PRODUCER_RECV_CMD | FSM_PRODUCER_RECV_PART_MESSAGE;
        return FSM_PRODUCER_RECV_CMD;
    }

    void ServerController::_send( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !Protocol::send( fd, packet ) ) return;

        switch( sess->fsm ) {
            case FSM_COMMON_SEND_CONFIRM_SECURE:
                sess->fsm = FSM_COMMON_RECV_CMD_GET_VERSION;
                break;
            case FSM_COMMON_SEND_VERSION:
                sess->fsm = FSM_COMMON_RECV_CMD_AUTH;
                break;
            case FSM_COMMON_SEND_CONFIRM_AUTH_GROUP:
                sess->fsm = FSM_GROUP_RECV_CMD;
                break;
            case FSM_COMMON_SEND_CONFIRM_AUTH_CONSUMER:
                sess->fsm = FSM_CONSUMER_RECV_CMD;
                break;
            case FSM_COMMON_SEND_CONFIRM_AUTH_PRODUCER:
                sess->fsm = FSM_PRODUCER_RECV_CMD;
                break;
            case FSM_GROUP_SEND:
            case FSM_GROUP_SEND_ERROR:
                sess->fsm = FSM_GROUP_RECV_CMD;
                break;
            case FSM_GROUP_SEND_ERROR_WITH_CLOSE:
                sess->fsm = FSM_GROUP_CLOSE;
                break;
            case FSM_CONSUMER_SEND:
            case FSM_CONSUMER_SEND_ERROR:
                sess->fsm = FSM_CONSUMER_RECV_CMD;
                break;
            case FSM_CONSUMER_SEND_ERROR_WITH_CLOSE:
                sess->fsm = FSM_CONSUMER_CLOSE;
                break;
            case FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE:
                sess->fsm = _getFSMAfterSendConfirmToConsumer( sess );
                break;
            case FSM_PRODUCER_SEND:
            case FSM_PRODUCER_SEND_ERROR:
                sess->fsm = FSM_PRODUCER_RECV_CMD;
                break;
            case FSM_PRODUCER_SEND_ERROR_WITH_CLOSE:
                sess->fsm = FSM_PRODUCER_CLOSE;
                break;
            case FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE:
                sess->fsm = _getFSMAfterSendConfirmToProducer( sess );
                break;
        }
    }

    void ServerController::_recvSecure( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isCheckNoSecure( sess->packet.get() ) ) throw util::Error::WRONG_CMD;

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_SECURE;

        _send( fd, sess );
    }

    void ServerController::_recvVersion( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isGetVersion( sess->packet.get() ) ) throw util::Error::WRONG_CMD;

        Protocol::prepareVersion( packet );
        sess->fsm = FSM_COMMON_SEND_VERSION;

        _send( fd, sess );
    }

    void ServerController::_authGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto password = Protocol::getPassword( packet );

        _access->authGroup( group, password, fd );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_GROUP;

        _send( fd, sess );
    }

    void ServerController::_authConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authConsumer( group, channel, login, password, fd );
        _q->joinConsumer( group, channel, fd );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_CONSUMER;

        _send( fd, sess );
    }

    void ServerController::_authProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );
        auto password = Protocol::getPassword( packet );

        _access->authProducer( group, channel, login, password, fd );
        _q->joinProducer( group, channel, fd );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_PRODUCER;

        _send( fd, sess );
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

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;

        _send( fd, sess );
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

        Protocol::prepareOk( packet );
        sess->fsm = FSM_CONSUMER_SEND;

        _send( fd, sess );
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

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;

        _send( fd, sess );
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

        Protocol::prepareOk( packet );
        sess->fsm = FSM_PRODUCER_SEND;

        _send( fd, sess );
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

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;

        _send( fd, sess );
    }

    void ServerController::_recvGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

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
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isPopMessage( packet ) ) {
        } else if( Protocol::isRemoveMessageByUUID( packet ) ) {
        } else if( Protocol::isOk( packet ) ) {
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvConsumerCmdPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isGetPartMessage( packet ) ) throw util::Error::WRONG_CMD;
    }

    void ServerController::_recvConsumerRemoveMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isRemoveMessage( packet ) ) throw util::Error::WRONG_CMD;
    }

    void ServerController::_recvProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyProducerPasswordCmd( fd, sess );
        } else if( Protocol::isPushMessage( packet ) ) {
        } else if( Protocol::isPushPublicMessage( packet ) ) {
        } else if( Protocol::isPushReplicaMessage( packet ) ) {
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvFromProducerPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
    }

    void ServerController::_recvFromProducerPartMessageNull( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
    }

    void ServerController::_sendToConsumerPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
    }

    void ServerController::_sendToConsumerPartMessageNull( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
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
                case FSM_GROUP_RECV_CMD:
                    _recvGroupCmd( fd, sess );
                    break;
                case FSM_CONSUMER_RECV_CMD:
                    _recvConsumerCmd( fd, sess );
                    break;
                case FSM_CONSUMER_RECV_CMD_PART_MESSAGE:
                    _recvConsumerCmdPartMessage( fd, sess );
                    break;
                case FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE:
                    _recvConsumerRemoveMessage( fd, sess );
                    break;
                case FSM_PRODUCER_RECV_CMD:
                    _recvProducerCmd( fd, sess );
                    break;
                case FSM_PRODUCER_RECV_PART_MESSAGE:
                    _recvFromProducerPartMessage( fd, sess );
                    break;
                case FSM_PRODUCER_RECV_PART_MESSAGE_NULL:
                    _recvFromProducerPartMessageNull( fd, sess );
                    break;
                default:
                    throw util::Error::WRONG_CMD;
            }
        } catch( util::Error::Err err ) {
            sess->fsm = _getFSMByError( sess->fsm, err );

            switch( sess->fsm ) {
                case FSM_COMMON_CLOSE:
                case FSM_GROUP_CLOSE:
                case FSM_CONSUMER_CLOSE:
                case FSM_PRODUCER_CLOSE:
                    _close( fd, sess );
                    break;
                default:
                    try {
                        Protocol::prepareError( sess->packet.get(), util::Error::getDescription( err ) );
                        _send( fd, sess );
                    } catch( ... ) {
                        _close( fd, sess );
                    }
                    break;
            }
        } catch( ... ) {
            _close( fd, sess );
        }

    }

    void ServerController::send( unsigned int fd ) {
        auto sess = _sessions[fd].get();

        try {
            switch( sess->fsm ) {
                case FSM_CONSUMER_SEND_PART_MESSAGE:
                    _sendToConsumerPartMessage( fd, sess );
                    break;
                case FSM_CONSUMER_SEND_PART_MESSAGE_NULL:
                    _sendToConsumerPartMessageNull( fd, sess );
                    break;
                default:
                    _send( fd, sess );
                    break;
            }
        } catch( ... ) {
            _close( fd, sess );
        }
    }

    void ServerController::disconnect( unsigned int fd ) {
        _sessions.erase( fd );
    }

    void ServerController::iteration() {
    }
}

#endif
 
