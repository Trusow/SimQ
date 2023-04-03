#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/file.hpp"
#include "../../util/uuid.hpp"
#include <list>

namespace simq::core::server {
    class Changes {
        public:
            enum Type {
                CREATE_GROUP,
                REMOVE_GROUP,
            };
        private:
            const char *defPath = "changes";

            struct ChangeFile {
                Type type;
                unsigned int values[16];
                unsigned int valuesLength;
            };
        public:

            struct Change: ChangeFile {
                void *data;
            };

        private:

            struct ChangeDisk {
                Change *change;
                char uuid[util::UUID::LENGTH];
            };

            std::list<Change *> listMemory;
            std::list<ChangeDisk> listDisk;

        public:
            Changes( const char *path );
            ~Changes();



            Change *addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] );
            Change *removeGroup( const char *group );

            bool isAddGroup( Change *change );
            bool isRemoveGroup( Change *change );

            const char *getGroup( Change *change );
            const char *getChannel( Change *change );
            const char *getLogin( Change *change );
            const util::Types::ChannelSettings *getChannelSettings( Change *change );
            const unsigned char *getPassword( Change *change );

            void push( Change *change );
            void pushDefered( Change *change );
            Change *pop();

            void free( Change *change );
    };
}

#endif
