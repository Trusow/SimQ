#ifndef SIMQ_CORE_SERVER_CHANGES
#define SIMQ_CORE_SERVER_CHANGES

#include "../../crypto/hash.hpp"
#include "../../util/types.h"
#include "../../util/file.hpp"
#include "../../util/fs.hpp"
#include "../../util/validation.hpp"
#include "../../util/uuid.hpp"
#include <list>
#include <string>
#include <mutex>
#include <arpa/inet.h>

namespace simq::core::server {
    class Changes {
        public:
            enum Type {
                CH_CREATE_GROUP,
                CH_REMOVE_GROUP,
            };
        private:
            const char *defPath = "changes";
            const unsigned int LENGTH_VALUES = 16;

            struct ChangeFile {
                unsigned int type;
                unsigned int values[16];
            };

        public:

            struct Change: ChangeFile {
                char *data;
            };

        private:

            char *_path = nullptr;

            struct ChangeDisk {
                Change *change;
                char uuid[util::UUID::LENGTH];
            };

            std::list<Change *> listMemory;
            std::list<ChangeDisk> listDisk;
            std::mutex m;

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
            void getPassword( Change *change, unsigned char password[crypto::HASH_LENGTH] );

            void push( Change *change );
            void pushDefered( Change *change );
            Change *pop();

            void free( Change *change );
    };

    Changes::Changes( const char *path ) {
        std::string strPath = path;
        strPath += "/";
        strPath += defPath;
        strPath += "/";
        auto _strPath = strPath.c_str();
        auto l = strPath.size();

        if( !util::FS::dirExists( _strPath ) ) {
            if( !util::FS::createDir( _strPath ) ) {
                throw util::Error::FS_ERROR;
            }
        }

        _path = new char[l+1]{};
        memcpy( _path, _strPath, l );
    }

    Changes::~Changes() {
        if( _path != nullptr ) {
            delete[] _path;
        }

        for( auto it = listMemory.begin(); it != listMemory.end(); it++ ) {
            delete[] (*it)->data;
            delete *it;
        }

        for( auto it = listDisk.begin(); it != listDisk.end(); it++ ) {
            delete[] (*it).change->data;
            delete (*it).change;
        }
    }

    Changes::Change *Changes::addGroup( const char *group, unsigned char password[crypto::HASH_LENGTH] ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        auto groupLength = strlen( group );

        auto change = new Change{};

        change->type = CH_CREATE_GROUP;
        change->values[0] = groupLength;
        change->values[1] = crypto::HASH_LENGTH;

        change->data = new char[groupLength+crypto::HASH_LENGTH+1]{};

        memcpy( change->data, group, groupLength );
        memcpy( &change->data[groupLength+1], password, crypto::HASH_LENGTH );

        return change;
    }

    Changes::Change *Changes::removeGroup( const char *group ) {
        if( !util::Validation::isGroupName( group ) ) {
            throw util::Error::WRONG_GROUP;
        }

        auto groupLength = strlen( group );

        auto change = new Change{};

        change->type = CH_REMOVE_GROUP;
        change->values[0] = groupLength;

        change->data = new char[groupLength+crypto::HASH_LENGTH+1]{};

        memcpy( change->data, group, groupLength );

        return change;
    }

    const char *Changes::getGroup( Change *change ) {
        return change->data;
    }

    const char *Changes::getChannel( Change *change ) {
        auto offset = change->values[0] + 1;
        return &change->data[offset];
    }

    const char *Changes::getLogin( Change *change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return &change->data[offset];
    }

    const util::Types::ChannelSettings *Changes::getChannelSettings( Change *change ) {
        auto offset = change->values[0] + change->values[1] + 2;
        return (util::Types::ChannelSettings *)&change->data[offset];
    }

    void Changes::getPassword( Change *change, unsigned char password[crypto::HASH_LENGTH] ) {
        if( isAddGroup( change ) ) {
            auto offset = change->values[0] + 1;
            memcpy( password, &change->data[offset], crypto::HASH_LENGTH );
        }
    }

    bool Changes::isAddGroup( Change *change ) {
        return change->type == CH_CREATE_GROUP;
    }

    bool Changes::isRemoveGroup( Change *change ) {
        return change->type == CH_REMOVE_GROUP;
    }

    void Changes::free( Change *change ) {
        if( change->data != nullptr ) {
            delete[] change->data;
        }
        delete change;
    }

    void Changes::push( Change *change ) {
        std::lock_guard<std::mutex> lock( m );

        listMemory.push_back( change );
    }

    void Changes::pushDefered( Change *change ) {

        ChangeFile ch;

        ch.type = htonl( change->type );

        auto length = 0;

        for( unsigned int i = 0; i < LENGTH_VALUES; i++ ) {
            length += change->values[i];
            if( change->values[i] ) {
                length += 1;
            }
            ch.values[i] = htonl( change->values[i] );
        }

        // TODO: Доработать хранение числовых данных в дате
        //
        // TODO: Проверка файла на уникальность

        length -= 1;

        char uuid[util::UUID::LENGTH+1];
        memset( uuid, 0, util::UUID::LENGTH+1 );
        util::UUID::generate( uuid );

        std::string tmpPath = _path;

        tmpPath += "_";
        tmpPath += uuid;

        auto file = util::File( tmpPath.c_str(), true );
        file.write( &ch, sizeof( ChangeFile ) );
        file.write( change->data, length );
        file.rename( uuid );
    }
}

#endif
