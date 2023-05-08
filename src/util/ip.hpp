#ifndef SIMQ_UTIL_IP 
#define SIMQ_UTIL_IP 

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

namespace simq::util {
    class IP {
        public:
            static unsigned int toInt( const char *ip );
            static void toString( unsigned int ip, char *str );
    };

    unsigned int IP::toInt( const char *ip ) {
        struct sockaddr_in sa;
        inet_pton( AF_INET, ip, &( sa.sin_addr ) );

        return sa.sin_addr.s_addr;
    }

    void IP::toString( unsigned int ip, char *str ) {
        struct in_addr ip_addr;
        ip_addr.s_addr = ip;

        auto _str = inet_ntoa( ip_addr );
        auto l = strlen( _str );

        memcpy( str, _str, l );
        str[l] = 0;
    }
}

#endif
