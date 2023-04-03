#ifndef SIMQ_UTIL_UUID 
#define SIMQ_UTIL_UUID 

#include <random>

namespace simq::util {
    class UUID {
        private:
            static char convertIntToHex( unsigned int value );
        public:
            static const unsigned int LENGTH = 36;
            static void generate( char *data );
    };

    char UUID::convertIntToHex( unsigned int value ) {
        if( value >= 0 && value <= 9 ) {
            return value + 48;
        } else if( value >= 10 && value <= 15 ) {
            return value + 87;
        }

        return 48;
    }

    void UUID::generate( char *data ) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        for (int i = 0; i < LENGTH; i++) {
            if( i == 8 || i == 13 || i == 18 || i == 23 ) {
                data[i] = '-';
            } else if( i == 14 ) {
                data[i] = '4';
            } else if( i == 19 ) {
                data[i] = convertIntToHex( dis2( gen ) );
            } else {
                data[i] = convertIntToHex( dis( gen ) );
            }
        }
    }
}

#endif
