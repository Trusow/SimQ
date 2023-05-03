#ifndef SIMQ_UTIL_VALIDATION 
#define SIMQ_UTIL_VALIDATION 

#include "uuid.hpp"
#include "types.h"
#include <string.h>
#include <thread>

namespace simq::util {
    class Validation {
        private:
            static bool isName( const char *name, unsigned int length );
        public:
            static bool isIPv4( const char *ip );
            static bool isGroupName( const char *name );
            static bool isChannelName( const char *name );
            static bool isConsumerName( const char *name );
            static bool isProducerName( const char *name );
            static bool isUUID( const char *name );
            static bool isPort( unsigned int port );
            static bool isCountThread( unsigned int count );
            static bool isUInt( const char *value );
            static bool isChannelLimitMessages( util::types::ChannelLimitMessages &limitMessages );
    };

    bool Validation::isIPv4( const char *ip ) {
        unsigned int accSeparator = 0;
        bool isMask = false;
        char ch;
        unsigned int partI = 0;
        unsigned int val = 0;
        for( int i = 0; ip[i] != 0; i++ ) {
            ch = ip[i];
     
            if( ch >= '0' && ch <= '9' ) {
                if( partI == 0 ) {
                    val = ch - '0';
                } else {
                    if( val == 0 ) {
                        return false;
                    }
     
                    val = val * 10 + ( ch - '0' );
     
                    if( !isMask && val > 255 || isMask && val > 32 ) {
                        return false;
                    }
                }
     
                partI++;
            } else if( ch == '.' ) {
                if( accSeparator == 3 || partI == 0 ) {
                    return false;
                }
                accSeparator++;
                partI = 0;
            } else if( ch == '/' ) {
                if( isMask || partI == 0 ) {
                    return false;
                }
                partI = 0;
                isMask = true;
            } else {
                return false;
            }
        }
     
        return accSeparator == 3 && partI != 0;
    }

    bool Validation::isUUID( const char *name ) {
        char ch;
        bool isNum, isWord;

        if( strlen( name ) != util::UUID::LENGTH ) {
            return false;
        }

        for (int i = 0; i < util::UUID::LENGTH; i++) {
            ch = name[i];
            if( i == 8 || i == 13 || i == 18 || i == 23 ) {
                if( ch != '-' ) {
                    return false;
                }
            } else if( i == 14 ) {
                if( ch != '4' ) {
                    return false;
                }
            } else {
                isWord = ch >= 'a' && ch <= 'z';
                isNum = ch >= '0' && ch <= '9';

                if( !isWord && !isNum ) {
                    return false;
                }
            }
        }

        return true;
    }

    bool Validation::isName( const char *name, unsigned int length ) {
        char ch;
        bool isNum, isWord;

        auto l = strlen( name );

        if( l > length || !l ) {
            return false;
        }

        for( unsigned int i = 0; name[i] != 0; i++ ) {
            if( i == length ) {
                return false;
            }

            ch = name[i];
            isNum = ch >= '0' && ch <= '9';
            isWord = ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';

            if( !isNum && !isWord ) {
                return false;
            }
        }

        return true;
    }

    bool Validation::isGroupName( const char *name ) {
        return isName( name, 32 );
    }

    bool Validation::isChannelName( const char *name ) {
        return isName( name, 32 );
    }

    bool Validation::isConsumerName( const char *name ) {
        return isName( name, 32 );
    }

    bool Validation::isProducerName( const char *name ) {
        return isName( name, 32 );
    }

    bool Validation::isPort( unsigned int port ) {
        return port > 0 && port <= 0xFF'FF;
    }

    bool Validation::isUInt( const char *value ) {
        unsigned long int v = 0;
        bool isStartNull = false;

        if( value[0] == 0 ) {
            return false;
        }

        for( unsigned int i = 0; value[i] != 0; i++ ) {
            char ch = value[i];
            if( ch >= '0' && ch <= '9' ) {
                if( ch == '0' && i == 0 ) {
                    isStartNull = true;
                }
                if( i != 0 && isStartNull ) {
                    return false;
                }

                v = v * 10 + ( ch - '0' );
                if( v > 0xFF'FF'FF'FF ) {
                    return false;
                }
            } else {
                return false;
            }
        }

        return true;
    }

    bool Validation::isCountThread( unsigned int count ) {
        if( count == 0 ) {
            return false;
        }

        const unsigned int hc = std::thread::hardware_concurrency();

        return count <= hc + hc / 2;
    }

    bool Validation::isChannelLimitMessages( util::types::ChannelLimitMessages &limitMessages ) {
        unsigned long int _size = limitMessages.maxMessagesOnDisk + limitMessages.maxMessagesInMemory;

        if( _size > 0xFF'FF'FF'FF || _size == 0 ) {
            return false;
        }


        if( limitMessages.minMessageSize == 0 || limitMessages.maxMessageSize == 0 ) {
            return false;
        }

        if( limitMessages.minMessageSize > limitMessages.maxMessageSize ) {
            return false;
        }

        return true;
    }
}

#endif
