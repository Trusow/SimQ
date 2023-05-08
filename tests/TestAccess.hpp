#ifndef SIMQ_TEST_ACCESS
#define SIMQ_TEST_ACCESS

#include <iostream>
#include "../src/core/server/access.hpp"
#include "../src/crypto/hash.hpp"
#include "../src/util/error.h"

namespace simq::test {
    class Access {
        private:
            void _printPassed();
            void _printFailed();

            void _runGroup();
            void _runGroupCreate();
            void _runGroupAuth();
            void _runGroupLogout();
            void _runGroupUpdatePassword();

            void _runChannel();
            void _runChannelCreate();
            void _runChannelRemove();
        public:
            void run();
    };

    void Access::_printPassed() {
        std::cout << "\x1b[32m";
        std::cout << "passed" << std::endl;
        std::cout << "\x1b[0m";
    }

    void Access::_printFailed() {
        std::cout << "\x1b[31m";
        std::cout << "failed" << std::endl;
        std::cout << "\x1b[0m";
    }

    void Access::_runGroupUpdatePassword() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "update password: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.updateGroupPassword( "123", password );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "update password not found group: ";

            core::server::Access access;
            access.updateGroupPassword( "123", password );
            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "check update password: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.checkUpdateMyGroupPassword( "123", 1 );

            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "check update password after logout: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.logoutGroup( "123", 1 );
            access.checkUpdateMyGroupPassword( "123", 1 );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_SESSION ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "check update password after renew group: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.removeGroup( "123" );
            access.addGroup( "123", password );
            access.checkUpdateMyGroupPassword( "123", 1 );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_SESSION ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "check update password after remove group: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.removeGroup( "123" );
            access.checkUpdateMyGroupPassword( "123", 1 );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "check update password after update password: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.updateGroupPassword( "123", password );
            access.checkUpdateMyGroupPassword( "123", 1 );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_SESSION ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runGroupLogout() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "logout: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.logoutGroup( "123", 1 );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "logout not found group: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.removeGroup( "123" );
            access.logoutGroup( "123", 1 );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "logout not found session: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.logoutGroup( "123", 1 );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runGroupAuth() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};
        unsigned char passwordWrong[simq::crypto::HASH_LENGTH]{};
        passwordWrong[0] = 'a';

        try {
            std::cout << "auth: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "auth with wrong password: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", passwordWrong, 1 );
            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::WRONG_PASSWORD ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "auth duplicate: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.authGroup( "123", password, 1 );
            access.authGroup( "123", password, 1 );
            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::DUPLICATE_SESSION ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "auth not found group: ";

            core::server::Access access;
            access.authGroup( "123", password, 1 );
            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runGroupCreate() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "create: ";

            core::server::Access access;
            access.addGroup( "123", password );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "create duplicate: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addGroup( "123", password );
            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::DUPLICATE_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "create after remove: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.removeGroup( "123" );
            access.addGroup( "123", password );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runGroup() {
        std::cout << "test groups" << std::endl;

        _runGroupCreate();

        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "remove: ";
            core::server::Access access;

            access.addGroup( "123", password );
            access.removeGroup( "123" );
            access.removeGroup( "123" );
            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        _runGroupAuth();
        _runGroupLogout();
        _runGroupUpdatePassword();

        std::cout << std::endl;
    }

    void Access::_runChannelRemove() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "remove: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addChannel( "123", "ch" );
            access.removeChannel( "123", "ch" );

            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "remove not found group: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addChannel( "123", "ch" );
            access.removeChannel( "122", "ch" );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "remove not found channel: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addChannel( "123", "ch" );
            access.removeChannel( "123", "ch" );
            access.removeChannel( "123", "ch" );

            _printPassed();
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runChannelCreate() {
        unsigned char password[simq::crypto::HASH_LENGTH]{};

        try {
            std::cout << "create: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addChannel( "123", "ch" );

            _printPassed();
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "create duplicate: ";

            core::server::Access access;
            access.addGroup( "123", password );
            access.addChannel( "123", "ch" );
            access.addChannel( "123", "ch" );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::DUPLICATE_CHANNEL ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }

        try {
            std::cout << "create not found group: ";

            core::server::Access access;
            access.addChannel( "123", "ch" );

            _printFailed();
        } catch( util::Error::Err err ) {
            if( err == util::Error::NOT_FOUND_GROUP ) {
                _printPassed();
            } else {
                _printFailed();
            }
        } catch( ... ) {
            _printFailed();
        }
    }

    void Access::_runChannel() {
        std::cout << "test channels" << std::endl;

        _runChannelCreate();
        _runChannelRemove();

        std::cout << std::endl;
    }

    void Access::run() {
        _runGroup();
        _runChannel();
        // TODO: Дописать тесты
    }
}

#endif
 
