#ifndef SIMQ_CORE_SERVER_INITIALIZATION
#define SIMQ_CORE_SERVER_INITIALIZATION

#include "access.hpp"
#include "store.hpp"
#include "changes.hpp"
#include "q/manager.hpp"
#include "logger.hpp"

namespace simq::core::server {
    class Initialization {
        private:
            Store *_store = nullptr;
            Access *_access = nullptr;
            Changes *_changes = nullptr;
            q::Manager *_q = nullptr;
        public:
            Initialization( Store &store, Access &access, Changes &changes, q::Manager &q );
            void pollChanges();
    };

    Initialization::Initialization( Store &store, Access &access, Changes &changes, q::Manager &q ) {
        _store = &store;
        _access = &access;
        _changes = &changes;
        _q = &q;

        std::vector<std::string> groups;
        _store->getGroups( groups );

        for( auto iGroup = 0; iGroup < groups.size(); iGroup++ ) {
            auto group = groups[iGroup].c_str();

            std::list<Logger::Detail> details;
            Logger::Detail det;
            det.name = "group";
            det.value = group;
            details.push_back( det );

            try {
                unsigned char password[crypto::HASH_LENGTH];

                _store->getGroupPassword( group, password );

                access.addGroup( group, password );
                q.addGroup( group );

                Logger::success(
                    Logger::OP_INITIALIZATION_GROUP,
                    0,
                    details,
                    simq::core::server::Logger::I_ROOT
                );

                std::vector<std::string> channels;

                _store->getChannels( group, channels );

                for( auto iChannel = 0; iChannel < channels.size(); iChannel++ ) {
                    auto channel = channels[iChannel].c_str();
                    details.clear();
                    try {

                        det.name = "channel";
                        det.value = channel;
                        details.push_back( det );

                        util::types::ChannelLimitMessages limitMessages{};
                        _store->getChannelLimitMessages( group, channel, limitMessages );

                        det.name = "minMessageSize";
                        det.value = std::to_string( limitMessages.minMessageSize );
                        details.push_back( det );

                        det.name = "maxMessageSize";
                        det.value = std::to_string( limitMessages.maxMessageSize );
                        details.push_back( det );

                        det.name = "maxMessagesInMemory";
                        det.value = std::to_string( limitMessages.maxMessagesInMemory );
                        details.push_back( det );

                        det.name = "maxMessagesOnDisk";
                        det.value = std::to_string( limitMessages.maxMessagesOnDisk );
                        details.push_back( det );

                        access.addChannel( group, channel );
                        //q.addChannel( group, channel, ".", limitMessages );

                        Logger::success(
                            Logger::OP_INITIALIZATION_CHANNEL,
                            0,
                            details,
                            simq::core::server::Logger::I_ROOT
                        );

                        std::vector<std::string> consumers;
                        std::vector<std::string> producers;

                        _store->getConsumers( group, channel, consumers );
                        _store->getProducers( group, channel, producers );

                        for( auto iUser = 0; iUser < consumers.size(); iUser++ ) {
                            auto user = consumers[iUser].c_str();
                            unsigned char passwordUser[crypto::HASH_LENGTH];

                            details.clear();
                            det.name = "login";
                            det.value = user;
                            details.push_back( det );

                            try {
                                _store->getConsumerPassword( group, channel, user, password );

                                access.addConsumer( group, channel, user, password );

                                Logger::success(
                                    Logger::OP_INITIALIZATION_CONSUMER,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            } catch( util::Error::Err err ) {
                                Logger::fail(
                                    Logger::OP_INITIALIZATION_CONSUMER,
                                    err,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            } catch( ... ) {
                                Logger::fail(
                                    Logger::OP_INITIALIZATION_CONSUMER,
                                    util::Error::UNKNOWN,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            }
                        }

                        for( auto iUser = 0; iUser < producers.size(); iUser++ ) {
                            auto user = producers[iUser].c_str();
                            unsigned char passwordUser[crypto::HASH_LENGTH];

                            details.clear();
                            det.name = "login";
                            det.value = user;
                            details.push_back( det );

                            try {
                                _store->getProducerPassword( group, channel, user, password );

                                access.addProducer( group, channel, user, password );

                                Logger::success(
                                    Logger::OP_INITIALIZATION_PRODUCER,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            } catch( util::Error::Err err ) {
                                Logger::fail(
                                    Logger::OP_INITIALIZATION_PRODUCER,
                                    err,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            } catch( ... ) {
                                Logger::fail(
                                    Logger::OP_INITIALIZATION_PRODUCER,
                                    util::Error::UNKNOWN,
                                    0,
                                    details,
                                    Logger::I_ROOT
                                );
                            }
                        }

                    } catch( util::Error::Err err ) {
                        Logger::fail(
                            Logger::OP_INITIALIZATION_CHANNEL,
                            err,
                            0,
                            details,
                            Logger::I_ROOT
                        );
                    } catch( ... ) {
                        Logger::fail(
                            Logger::OP_INITIALIZATION_CHANNEL,
                            util::Error::UNKNOWN,
                            0,
                            details,
                            Logger::I_ROOT
                        );
                    }
                }



            } catch( util::Error::Err err ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    err,
                    0,
                    details,
                    Logger::I_ROOT
                );
            } catch( ... ) {
                Logger::fail(
                    Logger::OP_INITIALIZATION_GROUP,
                    util::Error::UNKNOWN,
                    0,
                    details,
                    Logger::I_ROOT
                );
            }
        }
    }

    void Initialization::pollChanges() {
    }


}

#endif
