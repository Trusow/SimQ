#ifndef SIMQ_CORE_SERVER_Q_UUID
#define SIMQ_CORE_SERVER_Q_UUID

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <map>
#include <string>
#include <string.h>
#include "../../../util/uuid.hpp"
#include "../../../util/lock_atomic.hpp"
#include "../../../util/error.h"

namespace simq::core::server::q {
    class UUID {
        private:
            std::shared_timed_mutex _m;
            std::atomic_uint _countWrited;

            std::map<std::string, bool> _map;

            void _wait( std::atomic_uint &atom );
        public:
            void generate( char *uuid );
            void add( const char *uuid );
            bool check( const char *uuid );
            void free( const char *uuid );
    };

    void UUID::_wait( std::atomic_uint &atom ) {
        while( atom );
    }

    void UUID::generate( char *uuid ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        char _uuid[util::UUID::LENGTH+1]{};
        while( true ) {
            util::UUID::generate( _uuid );
            auto it = _map.find( _uuid );
            if( it == _map.end() ) {
                _map[_uuid] = true;
                memcpy( uuid, _uuid, util::UUID::LENGTH );
                return;
            }
        }
    }

    void UUID::add( const char *uuid ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        auto it = _map.find( uuid );
        if( it != _map.end() ) {
            throw util::Error::DUPLICATE_UUID;
        }

        _map[uuid] = true;
    }

    bool UUID::check( const char *uuid ) {
        _wait( _countWrited );
        std::shared_lock<std::shared_timed_mutex> lock( _m );

        auto it = _map.find( uuid );
        return it != _map.end();
    }

    void UUID::free( const char *uuid ) {
        util::LockAtomic lockAtomic( _countWrited );
        std::lock_guard<std::shared_timed_mutex> lock( _m );

        auto it = _map.find( uuid );

        if( it == _map.end() ) {
            return;
        }

        _map.erase( it );
    }
}

#endif
 
