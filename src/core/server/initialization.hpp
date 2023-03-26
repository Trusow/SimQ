#ifndef SIMQ_CORE_SERVER_INITIALIZATION
#define SIMQ_CORE_SERVER_INITIALIZATION

#include "access.hpp"
#include "q.hpp"

namespace simq::core::server {
    class Initialization {
        public:
            Initialization( const char *path, Access *access, Q *q );
            void addGroup( const char *group );
            void removeGroup( const char *group );
    };

    Initialization::Initialization( const char *path, Access *access, Q *q ) {
    }

    void Initialization::addGroup( const char *group ) {
    }

    void Initialization::removeGroup( const char *group ) {
    }
}

#endif
