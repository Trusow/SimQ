#include "sync.h" 

simq::util::Sync::Mutex *simq::util::Sync::getNewMutex( const char *name ) {
    auto *newMutex = new Mutex{};
    auto lengthName = strlen( name );
    newMutex->name = new char[lengthName+1]{};
    memcpy( newMutex->name, name, lengthName );

    return newMutex;
}

std::mutex &simq::util::Sync::get( const char *name ) {
    std::lock_guard<std::mutex> lock( m );

    if( list == nullptr ) {
        list = new Mutex*;
        list[0] = getNewMutex( name );
        listLength++;

        return list[0]->m;
    } else {
        for( int i = 0; i < listLength; i++ ) {
            if( strcmp( list[i]->name, name ) == 0 ) {
                return list[i]->m;
            }
        }

        auto *newMutex = getNewMutex( name );
        list = (Mutex **)realloc( list, sizeof( Mutex *) * ( listLength + 1 ) );
        list[listLength] = newMutex;
        listLength++;

        return newMutex->m;
    }
}
