#include "validation.h" 

bool simq::util::Validation::isIPv4( const char *ip ) {
    unsigned int i = 0;
    unsigned int accSeparator = 0;
    bool isMask = false;
    char ch;
    unsigned int partI = 0;
    unsigned int val = 0;
    for( i; ip[i] != 0; i++ ) {
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

bool simq::util::Validation::isName( const char *name, unsigned int length ) {
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

bool simq::util::Validation::isGroupName( const char *name ) {
    return isName( name, 32 );
}

bool simq::util::Validation::isChannelName( const char *name ) {
    return isName( name, 32 );
}

bool simq::util::Validation::isConsumerName( const char *name ) {
    return isName( name, 32 );
}

bool simq::util::Validation::isProducerName( const char *name ) {
    return isName( name, 32 );
}
