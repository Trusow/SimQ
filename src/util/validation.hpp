#ifndef SIMQ_UTIL_VALIDATION 
#define SIMQ_UTIL_VALIDATION 

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

    bool Validation::isName( const char *name, unsigned int length ) {
        char ch;
        bool isNum, isWord;

        for( unsigned int i = 0; name[i] != 0; i++ ) {
            if( i == length ) {
                return false;
            }

            ch = name[i];
            isNum = ch >= '0' && ch <= '9';
            isWord = ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';

            if( !isNum && isWord ) {
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
}

#endif
