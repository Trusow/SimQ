#ifndef SIMQ_CORE_SERVER_FSM
#define SIMQ_CORE_SERVER_FSM

namespace simq::core::server {
    class FSM {
        public:
            enum Code {
                COMMON_RECV_CMD_CHECK_SECURE,
                COMMON_SEND_CONFIRM_SECURE,
                COMMON_RECV_CMD_GET_VERSION,
                COMMON_SEND_VERSION,
                COMMON_RECV_CMD_AUTH,
                COMMON_SEND_CONFIRM_AUTH_GROUP,
                COMMON_SEND_CONFIRM_AUTH_CONSUMER,
                COMMON_SEND_CONFIRM_AUTH_PRODUCER,
                COMMON_SEND_ERROR_WITH_CLOSE,

                COMMON_CLOSE,



                GROUP_RECV_CMD,
                GROUP_SEND,
                GROUP_SEND_ERROR,
                GROUP_SEND_ERROR_WITH_CLOSE,

                GROUP_CLOSE,


                 
                CONSUMER_RECV_CMD,
                CONSUMER_SEND,
                CONSUMER_SEND_ERROR,
                CONSUMER_SEND_ERROR_WITH_CLOSE,

                CONSUMER_RECV_CMD_PART_MESSAGE,
                CONSUMER_SEND_MESSAGE_META,
                CONSUMER_SEND_PART_MESSAGE,
                CONSUMER_SEND_PART_MESSAGE_NULL,
                CONSUMER_SEND_CONFIRM_PART_MESSAGE,
                CONSUMER_SEND_CONFIRM_PART_MESSAGE_END,
                CONSUMER_RECV_CMD_REMOVE_MESSAGE,

                CONSUMER_CLOSE,


                PRODUCER_RECV_CMD,
                PRODUCER_SEND,
                PRODUCER_SEND_MESSAGE_META,
                PRODUCER_SEND_ERROR,
                PRODUCER_SEND_ERROR_WITH_CLOSE,

                PRODUCER_RECV_PART_MESSAGE,
                PRODUCER_RECV_PART_MESSAGE_NULL,
                PRODUCER_SEND_CONFIRM_PART_MESSAGE,
                PRODUCER_SEND_CONFIRM_PART_MESSAGE_END,

                PRODUCER_CLOSE,
            };

            static Code getNextCodeAfterSend( Code code );
            static bool isClose( Code code );
            static bool isConsumerClose( Code code );
            static bool isConsumer( Code code );
    };

    FSM::Code FSM::getNextCodeAfterSend( Code code ) {
        switch( code ) {
            case COMMON_SEND_CONFIRM_SECURE:
                return COMMON_RECV_CMD_GET_VERSION;
            case COMMON_SEND_VERSION:
                return COMMON_RECV_CMD_AUTH;
            case COMMON_SEND_CONFIRM_AUTH_GROUP:
                return GROUP_RECV_CMD;
                break;
            case COMMON_SEND_CONFIRM_AUTH_CONSUMER:
                return CONSUMER_RECV_CMD;
            case COMMON_SEND_CONFIRM_AUTH_PRODUCER:
                return PRODUCER_RECV_CMD;
            case GROUP_SEND:
            case GROUP_SEND_ERROR:
                return GROUP_RECV_CMD;
            case GROUP_SEND_ERROR_WITH_CLOSE:
                return GROUP_CLOSE;
            case CONSUMER_SEND:
            case CONSUMER_SEND_ERROR:
                return CONSUMER_RECV_CMD;
            case CONSUMER_SEND_MESSAGE_META:
                return CONSUMER_RECV_CMD_PART_MESSAGE;
            case CONSUMER_SEND_ERROR_WITH_CLOSE:
                return CONSUMER_CLOSE;
            case CONSUMER_SEND_CONFIRM_PART_MESSAGE:
                return CONSUMER_RECV_CMD_PART_MESSAGE;
            case CONSUMER_SEND_CONFIRM_PART_MESSAGE_END:
                return CONSUMER_RECV_CMD_REMOVE_MESSAGE;
            case PRODUCER_SEND:
            case PRODUCER_SEND_ERROR:
                return PRODUCER_RECV_CMD;
            case PRODUCER_SEND_MESSAGE_META:
                return PRODUCER_RECV_PART_MESSAGE;
            case PRODUCER_SEND_ERROR_WITH_CLOSE:
                return PRODUCER_CLOSE;
            case PRODUCER_SEND_CONFIRM_PART_MESSAGE:
                return PRODUCER_RECV_PART_MESSAGE;
            case PRODUCER_SEND_CONFIRM_PART_MESSAGE_END:
                return PRODUCER_RECV_CMD;
            default:
                return code;
        }
    }

    bool FSM::isClose( Code code ) {
        switch( code ) {
            case COMMON_CLOSE:
            case GROUP_CLOSE:
            case PRODUCER_CLOSE:
            case CONSUMER_CLOSE:
                return true;
            default:
                return false;
        }
    }

    bool FSM::isConsumer( Code code ) {
        switch( code ) {
                case CONSUMER_RECV_CMD:
                case CONSUMER_SEND:
                case CONSUMER_SEND_ERROR:
                case CONSUMER_SEND_ERROR_WITH_CLOSE:
                case CONSUMER_RECV_CMD_PART_MESSAGE:
                case CONSUMER_SEND_MESSAGE_META:
                case CONSUMER_SEND_PART_MESSAGE:
                case CONSUMER_SEND_PART_MESSAGE_NULL:
                case CONSUMER_SEND_CONFIRM_PART_MESSAGE:
                case CONSUMER_SEND_CONFIRM_PART_MESSAGE_END:
                case CONSUMER_RECV_CMD_REMOVE_MESSAGE:
                case CONSUMER_CLOSE:
                    return true;
                default:
                    return false;
        }
    }
}

#endif

