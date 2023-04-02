#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include <string>
#include "../../crypto/hash.hpp"

namespace simq::core::server {
    class Changes {
        public:
            Changes( const char *path );
            ~Changes();

            enum Type {
                CREATE_GROUP,
                REMOVE_GROUP,
            };

            struct Change {
                Type type;
                unsigned int valuesChar[8][32];
                unsigned int valuesCharLength[8];
                unsigned int valuesUInt[8];
                unsigned int customDataLength;
            };

            void addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH], Change &change );
            void removeGroup( const char *group, Change &change );

            bool isAddGroup( Change &change );
            void getParamsAddGroup( std::string &group, unsigned char password[crypto::HASH_LENGTH] );

            bool isRemoveGroup( Change &change );
            void getParamsRemoveGroup( std::string &group );

            void push( Change &change );
            void pushDefered( Change &change );
            void pop( Change &change );
    };
}

#endif
