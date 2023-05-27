#ifndef SIMQ_CORE_SERVER_SERVER_CONTROLLER
#define SIMQ_CORE_SERVER_SERVER_CONTROLLER

#include <unordered_map>
#include <memory>
#include <list>
#include <unistd.h>
#include <string.h>
#include "server/callbacks.h"
#include "../../util/error.h"
#include "../../util/types.h"
#include "../../util/uuid.hpp"
#include "access.hpp"
#include "store.hpp"
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
                FSM_CONSUMER_SEND_MESSAGE_META,
                FSM_CONSUMER_SEND_PART_MESSAGE,
                FSM_CONSUMER_SEND_PART_MESSAGE_NULL,
                FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE,
                FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE_END,
                FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE,

                FSM_CONSUMER_CLOSE,
                 

                FSM_PRODUCER_RECV_CMD,
                FSM_PRODUCER_SEND,
                FSM_PRODUCER_SEND_MESSAGE_META,
                FSM_PRODUCER_SEND_ERROR,
                FSM_PRODUCER_SEND_ERROR_WITH_CLOSE,
                 
                FSM_PRODUCER_RECV_PART_MESSAGE,
                FSM_PRODUCER_RECV_PART_MESSAGE_NULL,
                FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE,
                FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE_END,
                 
                FSM_PRODUCER_CLOSE,
            };

            enum Type {
                TYPE_COMMON,
                TYPE_GROUP,
                TYPE_CONSUMER,
                TYPE_PRODUCER,
            };
            
            struct Session {
                FSM fsm;
                Type type;
             
                std::unique_ptr<char[]> authData;
                std::unique_ptr<Protocol::Packet> packet;
                std::unique_ptr<Protocol::BasePacket> packetMsg;
                unsigned short int offsetChannel;
                unsigned short int offsetLogin;
             
                unsigned int ip;
                unsigned int lastTS;
             
                unsigned int msgID;
                unsigned int lengthMessage;
                unsigned int sendLengthMessage;
                bool isPublicMessage;
            };

            std::map<unsigned int, std::unique_ptr<Session>> _sessions;
            Access *_access = nullptr;
            q::Manager *_q = nullptr;
            Changes *_changes = nullptr;
            Store *_store = nullptr;

            FSM _getFSMByError( Session *sess, util::Error::Err err );

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
            void _recvConsumerRemoveMessageCmd( unsigned int fd, Session *sess );
            void _recvProducerCmd( unsigned int fd, Session *sess );
            void _recvFromProducerPartMessage( unsigned int fd, Session *sess );
            void _recvFromProducerPartMessageNull( unsigned int fd, Session *sess );
            void _sendToConsumerPartMessage( unsigned int fd, Session *sess );
            void _sendToConsumerPartMessageNull( unsigned int fd, Session *sess );
            void _send( unsigned int fd, Session *sess );

            void _getChannelsCmd( unsigned int fd, Session *sess );
            void _getChannelLimitMessagesCmd( unsigned int fd, Session *sess );
            void _getConsumersCmd( unsigned int fd, Session *sess );
            void _getProducersCmd( unsigned int fd, Session *sess );
            void _addChannelCmd( unsigned int fd, Session *sess );
            void _updateChannelLimitMessagesCmd( unsigned int fd, Session *sess );
            void _removeChannelCmd( unsigned int fd, Session *sess );
            void _addConsumerCmd( unsigned int fd, Session *sess );
            void _removeConsumerCmd( unsigned int fd, Session *sess );
            void _addProducerCmd( unsigned int fd, Session *sess );
            void _removeProducerCmd( unsigned int fd, Session *sess );

            void _popMessageCmd( unsigned int fd, Session *sess );
            void _removeMessageByUUIDCmd( unsigned int fd, Session *sess );

            void _pushMessageCmd( unsigned int fd, Session *sess );
            void _pushPublicMessageCmd( unsigned int fd, Session *sess );
            void _pushReplicaMessageCmd( unsigned int fd, Session *sess );


            void _copyAuthData(
                Session *sess,
                const char *group,
                const char *channel = nullptr,
                const char *login = nullptr
            );

        public:
            void connect( unsigned int fd, unsigned int ip );
            void recv( unsigned int fd );
            void send( unsigned int fd );
            void disconnect( unsigned int fd );
            void iteration();
    };

    ServerController::FSM ServerController::_getFSMByError( Session *sess, util::Error::Err err ) {
        switch( sess->type ) {
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
        char *group, *channel, *login;

        switch( sess->type ) {
            case TYPE_GROUP:
                _access->logoutGroup( sess->authData.get(), fd );
                break;
            case TYPE_CONSUMER:
                group = sess->authData.get();
                channel = &sess->authData.get()[sess->offsetChannel];
                login = &sess->authData.get()[sess->offsetLogin];

                _access->logoutConsumer( group, channel, login, fd );
                if( sess->msgID != 0 && !sess->isPublicMessage ) {
                    _q->revertMessage( group, channel, fd, sess->msgID );
                }
                _q->leaveConsumer( group, channel, fd );
                break;
            case TYPE_PRODUCER:
                group = sess->authData.get();
                channel = &sess->authData.get()[sess->offsetChannel];
                login = &sess->authData.get()[sess->offsetLogin];

                _access->logoutProducer( group, channel, login, fd );
                if( sess->msgID != 0 ) {
                    _q->removeMessage( group, channel, fd, sess->msgID );
                }
                _q->leaveProducer( group, channel, fd );
                break;
        }

        _sessions.erase( fd );
    }

    bool ServerController::_recvToPacket( unsigned int fd, Protocol::Packet *packet ) {
        Protocol::recv( fd, packet );

        return Protocol::isFull( packet );
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
            case FSM_CONSUMER_SEND_MESSAGE_META:
                sess->fsm = FSM_CONSUMER_RECV_CMD_PART_MESSAGE;
                break;
            case FSM_CONSUMER_SEND_ERROR_WITH_CLOSE:
                sess->fsm = FSM_CONSUMER_CLOSE;
                break;
            case FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE:
                sess->fsm = FSM_CONSUMER_RECV_CMD_PART_MESSAGE;
                break;
            case FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE_END:
                sess->fsm = FSM_CONSUMER_RECV_CMD_REMOVE_MESSAGE;
                break;
            case FSM_PRODUCER_SEND:
            case FSM_PRODUCER_SEND_ERROR:
                sess->fsm = FSM_PRODUCER_RECV_CMD;
                break;
            case FSM_PRODUCER_SEND_MESSAGE_META:
                sess->fsm = FSM_PRODUCER_RECV_PART_MESSAGE;
                break;
            case FSM_PRODUCER_SEND_ERROR_WITH_CLOSE:
                sess->fsm = FSM_PRODUCER_CLOSE;
                break;
            case FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE:
                sess->fsm = FSM_PRODUCER_RECV_PART_MESSAGE;
                break;
            case FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE_END:
                sess->fsm = FSM_PRODUCER_RECV_CMD;
                break;
        }

        switch( sess->fsm ) {
            case FSM_COMMON_CLOSE:
            case FSM_GROUP_CLOSE:
            case FSM_CONSUMER_CLOSE:
            case FSM_PRODUCER_CLOSE:
                _close( fd, sess );
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

    void ServerController::_copyAuthData(
        Session *sess,
        const char *group,
        const char *channel,
        const char *login
    ) {
        auto lGroup = strlen( group );

        if( channel == nullptr ) {
            sess->authData = std::make_unique<char[]>( lGroup + 1 );
            memcpy( sess->authData.get(), group, lGroup );
            return;
        }

        auto lChannel = strlen( channel );
        auto lLogin = strlen( login );

        sess->authData = std::make_unique<char[]>( lGroup + lChannel + lLogin + 3 );

        sess->offsetChannel = lGroup+1;
        sess->offsetLogin = lGroup+lChannel+2;

        memcpy( sess->authData.get(), group, lGroup );
        memcpy( &sess->authData.get()[sess->offsetChannel], channel, lChannel );
        memcpy( &sess->authData.get()[sess->offsetLogin], login, lLogin );
    }

    void ServerController::_authGroupCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = Protocol::getGroup( packet );
        auto password = Protocol::getPassword( packet );

        _access->authGroup( group, password, fd );
        _copyAuthData( sess, group );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_GROUP;
        sess->type = TYPE_GROUP;

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
        _copyAuthData( sess, group, channel, login );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_CONSUMER;
        sess->type = TYPE_CONSUMER;

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
        _copyAuthData( sess, group, channel, login );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_COMMON_SEND_CONFIRM_AUTH_PRODUCER;
        sess->type = TYPE_PRODUCER;

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
            _getChannelsCmd( fd, sess );
        } else if( Protocol::isGetChannelLimitMessages( packet ) ) {
            _getChannelLimitMessagesCmd( fd, sess );
        } else if( Protocol::isGetConsumers( packet ) ) {
            _getConsumersCmd( fd, sess );
        } else if( Protocol::isGetProducers( packet ) ) {
            _getProducersCmd( fd, sess );
        } else if( Protocol::isAddChannel( packet ) ) {
            _addChannelCmd( fd, sess );
        } else if( Protocol::isUpdateChannelLimitMessages( packet ) ) {
            _updateChannelLimitMessagesCmd( fd, sess );
        } else if( Protocol::isRemoveChannel( packet ) ) {
            _removeChannelCmd( fd, sess );
        } else if( Protocol::isAddConsumer( packet ) ) {
            _addConsumerCmd( fd, sess );
        } else if( Protocol::isUpdateConsumerPassword( packet ) ) {
            _updateConsumerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveConsumer( packet ) ) {
            _removeConsumerCmd( fd, sess );
        } else if( Protocol::isAddProducer( packet ) ) {
            _addProducerCmd( fd, sess );
        } else if( Protocol::isUpdateProducerPassword( packet ) ) {
            _updateProducerPasswordCmd( fd, sess );
        } else if( Protocol::isRemoveProducer( packet ) ) {
            _removeProducerCmd( fd, sess );
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
            _popMessageCmd( fd, sess );
        } else if( Protocol::isRemoveMessageByUUID( packet ) ) {
            _removeMessageByUUIDCmd( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvConsumerCmdPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isGetPartMessage( packet ) ) throw util::Error::WRONG_CMD;

        sess->fsm = FSM_CONSUMER_SEND_PART_MESSAGE;
        _sendToConsumerPartMessage( fd, sess );
    }

    void ServerController::_recvConsumerRemoveMessageCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( !Protocol::isRemoveMessage( packet ) ) throw util::Error::WRONG_CMD;

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkPopMessage( group, channel, login, fd );
        _q->removeMessage( group, channel, fd, sess->msgID );
        sess->msgID = 0;

        Protocol::prepareOk( packet );
        _send( fd, sess );
    }

    void ServerController::_recvProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        if( !_recvToPacket( fd, packet ) ) return;

        if( Protocol::isUpdatePassword( packet ) ) {
            _updateMyProducerPasswordCmd( fd, sess );
        } else if( Protocol::isPushMessage( packet ) ) {
            _pushMessageCmd( fd, sess );
        } else if( Protocol::isPushPublicMessage( packet ) ) {
            _pushPublicMessageCmd( fd, sess );
        } else if( Protocol::isPushReplicaMessage( packet ) ) {
            _pushReplicaMessageCmd( fd, sess );
        } else {
            throw util::Error::WRONG_CMD;
        }
    }

    void ServerController::_recvFromProducerPartMessage( unsigned int fd, Session *sess ) {
        auto packetMsg = sess->packetMsg.get();
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        try {
            _access->checkPushMessage( group, channel, login, fd );
            auto l = _q->recv( group, channel, fd, sess->msgID );
            Protocol::addWRLength( packetMsg, l );

            bool isSend = false;
            if( Protocol::isFull( packetMsg ) ) {
                sess->msgID = 0;
                sess->fsm = FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE_END;
                isSend = true;
            } else if( Protocol::isFullPart( packetMsg ) ) {
                sess->fsm = FSM_PRODUCER_SEND_CONFIRM_PART_MESSAGE;
                isSend = true;
            }

            if( !isSend ) return;

            Protocol::prepareOk( packet );
            _send( fd, sess );
        } catch( util::Error::Err err ) {
            if( err == util::Error::SOCKET ) {
                _close( fd, sess );
            } else {
                sess->fsm = FSM_PRODUCER_RECV_PART_MESSAGE_NULL;
                _recvFromProducerPartMessageNull( fd, sess );
            }
        } catch( ... ) {
            _close( fd, sess );
        }

    }

    void ServerController::_recvFromProducerPartMessageNull( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto residue = Protocol::getResiduePart( packetMsg );
        auto data = std::make_unique<char[]>( residue );
        auto l = ::recv( fd, data.get(), residue, MSG_NOSIGNAL );

        if( l == -1 ) {
            if( errno != EAGAIN ) {
                _close( fd, sess );
            }
            return;
        }

        Protocol::addWRLength( packetMsg, l );

        if( Protocol::isFullPart( packetMsg ) ) {
            Protocol::prepareError( packet, util::Error::getDescription( util::Error::ACCESS_DENY ) );

            sess->fsm = FSM_PRODUCER_SEND_ERROR_WITH_CLOSE;
            _send( fd, sess );
        }
    }

    void ServerController::_sendToConsumerPartMessage( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        try {
            _access->checkPopMessage( group, channel, login, fd );
            auto l = _q->send( group, channel, fd, sess->msgID );
            Protocol::addWRLength( packetMsg, l );

            if( Protocol::isFull( packetMsg ) ) {
                Protocol::prepareOk( packet );
                sess->fsm = FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE_END;
                _send( fd, sess );
            } else if( Protocol::isFullPart( packetMsg ) ) {
                Protocol::prepareOk( packet );
                sess->fsm = FSM_CONSUMER_SEND_CONFIRM_PART_MESSAGE;
                _send( fd, sess );
            }
        } catch( util::Error::Err err ) {
            if( err == util::Error::SOCKET ) {
                _close( fd, sess );
            } else {
                sess->fsm = FSM_CONSUMER_SEND_PART_MESSAGE_NULL;
                _sendToConsumerPartMessageNull( fd, sess );
            }
        } catch( ... ) {
            _close( fd, sess );
        }
    }

    void ServerController::_sendToConsumerPartMessageNull( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto residue = Protocol::getResiduePart( packetMsg );
        auto data = std::make_unique<char[]>( residue );
        auto l = ::send( fd, data.get(), residue, MSG_NOSIGNAL );

        if( l == -1 ) {
            if( errno != EAGAIN ) {
                _close( fd, sess );
            }
            return;
        }

        Protocol::addWRLength( packetMsg, l );

        if( Protocol::isFullPart( packetMsg ) ) {
            Protocol::prepareError( packet, util::Error::getDescription( util::Error::ACCESS_DENY ) );

            sess->fsm = FSM_CONSUMER_SEND_ERROR_WITH_CLOSE;
            _send( fd, sess );
        }
    }

    void ServerController::_getChannelsCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();

        _access->checkToGroup( group, fd );
        
        std::list<std::string> channels;
        _store->getChannels( group, channels );

        Protocol::prepareStringList( packet, channels );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_getChannelLimitMessagesCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );

        _access->checkToChannel( sess->authData.get(), channel, fd );

        util::types::ChannelLimitMessages limitMessages;
        _store->getChannelLimitMessages( group, channel, limitMessages ); 

        Protocol::prepareChannelLimitMessages( packet, limitMessages );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_getConsumersCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );

        _access->checkToChannel( sess->authData.get(), channel, fd );

        std::list<std::string> consumers;
        _store->getConsumers( group, channel, consumers );

        Protocol::prepareStringList( packet, consumers );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_getProducersCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );

        _access->checkToChannel( sess->authData.get(), channel, fd );

        std::list<std::string> producers;
        _store->getProducers( group, channel, producers );

        Protocol::prepareStringList( packet, producers );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_addChannelCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        util::types::ChannelLimitMessages limitMessages;
        Protocol::getChannelLimitMessages( packet, limitMessages );

        _access->checkAddChannel( group, fd, channel );

        auto change = _changes->addChannel( group, channel, &limitMessages );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_updateChannelLimitMessagesCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        util::types::ChannelLimitMessages limitMessages;
        Protocol::getChannelLimitMessages( packet, limitMessages );

        _access->checkToChannel( group, channel, fd );

        auto change = _changes->updateChannelLimitMessages( group, channel, &limitMessages );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_removeChannelCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );

        _access->checkToChannel( group, channel, fd );

        auto change = _changes->removeChannel( group, channel  );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_addConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );
        auto password = Protocol::getPassword( packet );

        _access->checkAddConsumer( group, fd, channel, login );

        auto change = _changes->addConsumer( group, channel, login, password );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_removeConsumerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getConsumer( packet );

        _access->checkRemoveConsumer( group, fd, channel, login );

        auto change = _changes->removeConsumer( group, channel, login );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_addProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );
        auto password = Protocol::getPassword( packet );

        _access->checkAddProducer( group, fd, channel, login );

        auto change = _changes->addProducer( group, channel, login, password );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_removeProducerCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = Protocol::getChannel( packet );
        auto login = Protocol::getProducer( packet );

        _access->checkRemoveProducer( group, fd, channel, login );

        auto change = _changes->removeProducer( group, channel, login );
        _changes->push( std::move( change ) );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_GROUP_SEND;
        _send( fd, sess );
    }

    void ServerController::_popMessageCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];
        char uuid[util::UUID::LENGTH]{};
        unsigned int length;

        _access->checkPopMessage( group, channel, login, fd );
        auto id = _q->popMessage( group, channel, fd, length, uuid );

        if( id == 0 ) {
            Protocol::prepareNoneMessageMetaPop( packet );
            sess->fsm = FSM_CONSUMER_SEND;
            _send( fd, sess );
            return;
        }

        Protocol::setLength( packetMsg, length );
        sess->fsm = FSM_CONSUMER_SEND_MESSAGE_META;
        sess->msgID = id;

        if( uuid[0] ) {
            sess->isPublicMessage = false;
            Protocol::prepareMessageMetaPop( packet, length, uuid );
        } else {
            sess->isPublicMessage = true;
            Protocol::preparePublicMessageMetaPop( packet, length );
        }

        _send( fd, sess );
    }

    void ServerController::_removeMessageByUUIDCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();

        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        auto uuid = Protocol::getUUID( packet );

        _access->checkPopMessage( group, channel, login, fd );
        _q->removeMessage( group, channel, fd, uuid );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_CONSUMER_SEND;
        _send( fd, sess );
    }

    void ServerController::_pushMessageCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto length = Protocol::getLength( packet );
        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkPushMessage( group, channel, login, fd );
        char uuid[util::UUID::LENGTH];

        sess->msgID = _q->createMessageForQ( group, channel, fd, length, uuid );
        Protocol::setLength( packetMsg, length );

        Protocol::prepareMessageMetaPush( packet, uuid );
        sess->fsm = FSM_PRODUCER_SEND_MESSAGE_META;

        _send( fd, sess );
    }

    void ServerController::_pushPublicMessageCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto length = Protocol::getLength( packet );
        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkPushMessage( group, channel, login, fd );

        sess->msgID = _q->createMessageForBroadcast( group, channel, fd, length );
        Protocol::setLength( packetMsg, length );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_PRODUCER_SEND_MESSAGE_META;

        _send( fd, sess );
    }
    
    void ServerController::_pushReplicaMessageCmd( unsigned int fd, Session *sess ) {
        auto packet = sess->packet.get();
        auto packetMsg = sess->packetMsg.get();

        auto length = Protocol::getLength( packet );
        auto uuid = Protocol::getUUID( packet );
        auto group = sess->authData.get();
        auto channel = &sess->authData.get()[sess->offsetChannel];
        auto login = &sess->authData.get()[sess->offsetLogin];

        _access->checkPushMessage( group, channel, login, fd );

        sess->msgID = _q->createMessageForReplication( group, channel, fd, length, uuid );
        Protocol::setLength( packetMsg, length );

        Protocol::prepareOk( packet );
        sess->fsm = FSM_PRODUCER_SEND_MESSAGE_META;

        _send( fd, sess );
    }

    void ServerController::connect( unsigned int fd, unsigned int ip ) {
        auto sess = std::make_unique<Session>();
        sess->fsm = FSM_COMMON_RECV_CMD_CHECK_SECURE;
        sess->type = TYPE_COMMON;
        sess->packet = std::make_unique<Protocol::Packet>();
        sess->packetMsg = std::make_unique<Protocol::BasePacket>();
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
                    _recvConsumerRemoveMessageCmd( fd, sess );
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
            sess->fsm = _getFSMByError( sess, err );

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
 
