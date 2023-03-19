#include "lock.h" 

simq::util::Lock::Lock() {
}

simq::util::Lock::Lock( std::mutex &m ) {
    m.lock();
    _m = &m;
}

simq::util::Lock::~Lock() {
    if( _m == nullptr ) {
        return;
    }

    _m->unlock();
}

void simq::util::Lock::lock( std::mutex &m ) {
    m.lock();
    _m = &m;
}
