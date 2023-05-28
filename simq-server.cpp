#include "src/core/server/cli/manager.hpp"
#include "src/core/server/logger.hpp"
#include "src/util/error.h"
#include "src/core/server/cli_controller.hpp"
#include "src/core/server/initialization.hpp"
#include "src/core/server/server/manager.hpp"
#include "src/core/server/server_controller.hpp"
#include <thread>
#include <list>
#include <string>
#include <atomic>
#include <mutex>

void startManager( const char *path = "." ) {
    simq::core::server::Store store( path );
    simq::core::server::Changes changes( path );
    simq::core::server::CLIController controller( &store, &changes );

    simq::core::server::CLI::Manager manger( &controller );

}

std::atomic_bool isPassedStartServer{ true };
std::mutex mError;

void startServer(
    simq::core::server::Store *store,
    simq::core::server::Access *access,
    simq::core::server::Changes *changes,
    simq::core::server::q::Manager *q
) {
    if( !isPassedStartServer ) {
        return;
    }


    try {
        simq::core::server::server::Manager server( store->getPort() );
        simq::core::server::ServerController controller( store, access, changes, q );

        server.bindController( &controller );

        std::list<simq::core::server::Logger::Detail> list;
        simq::core::server::Logger::success( simq::core::server::Logger::OP_START_SERVER, 0, list );

        server.run();
    } catch( ... ) {
        std::lock_guard<std::mutex> lock( mError );

        if( !isPassedStartServer ) {
            return;
        }

        std::list<simq::core::server::Logger::Detail> list;
        simq::core::server::Logger::fail(
            simq::core::server::Logger::OP_START_SERVER,
            simq::util::Error::SOCKET,
            0,
            list
        );

        isPassedStartServer = false;
    }
}

void startInit( const char *path = "." ) {
    simq::core::server::Access access;
    simq::core::server::q::Manager q;

    simq::core::server::Initialization ini( path, access, q );

    ini.pollChanges();

    auto changes = ini.getChanges();
    auto store = ini.getStore();

    for( unsigned int i = 0; i < store->getCountThreads(); i++ ) {
        std::thread t( startServer, store, &access, changes, &q );
        t.detach();
    }

    std::chrono::milliseconds ts( 50 );

    while( true ) {
        if( !isPassedStartServer ) {
            break;
        }
        ini.pollChanges();
        std::this_thread::sleep_for( ts );
    }

}

int main( int argc, char *argv[] ) {
    bool isManager = false;
    std::string path = ".";

    for( unsigned int i = 1; i < argc; i++ ) {
        std::string val = std::string( argv[i] );

        if( val == "manager" ) {
            isManager = true;
        }
    }

    if( isManager ) {
        startManager( path.c_str() );
    } else {
        startInit( path.c_str() );
    }
}
