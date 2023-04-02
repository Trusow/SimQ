#ifndef SIMQ_UTIL_FS
#define SIMQ_UTIL_FS

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "error.h"

namespace simq::util {
    class FS {
        private:
        enum Type {
            _FILE,
            _DIR,
        };
        static void _list( const char *path, std::vector<std::string> &v, Type t );
        public:
        static bool dirExists( const char *path );
        static bool fileExists( const char *path );
        static bool createDir( const char *path );
        static void removeDir( const char *path );
        static bool removeFile( const char *path );
        static void dirs( const char *path, std::vector<std::string> &v );
        static void files( const char *path, std::vector<std::string> &v );
    };

    bool FS::dirExists( const char *path ) {
        auto *dir = opendir( path );

        if( dir == nullptr ) {
            return false;
        }

        closedir( dir );

        return true;
    }

    bool FS::fileExists( const char *path ) {
        auto *file = fopen( path, "r" );

        if( file == nullptr ) {
            return false;
        }

        fclose( file );

        return true;
    }

    bool FS::createDir( const char *path ) {
        return mkdir( path, 0700 ) == 0;
    }

    void FS::removeDir( const char *path ) {
        std::filesystem::remove_all( path );
    }

    bool FS::removeFile( const char *path ) {
        return unlink( path ) == 0;
    }

    void FS::_list( const char *path, std::vector<std::string> &v, Type t ) {
        auto *dir = opendir( path );

        if( dir == nullptr ) {
            throw util::Error::FS_ERROR;
        }
     
        dirent *pDirent;

        while (( pDirent = readdir( dir ) ) != nullptr) {
            bool isFile = t == Type::_FILE && pDirent->d_type == DT_REG;
            bool isDir = t == Type::_DIR && pDirent->d_type == DT_DIR;
            if(  isFile || isDir ) {

                v.push_back( pDirent->d_name );
            }
        }

        closedir( dir );

        std::sort( v.begin(), v.end() );
    }

    void FS::dirs( const char *path, std::vector<std::string> &v ) {
        _list( path, v, Type::_DIR );
    }

    void FS::files( const char *path, std::vector<std::string> &v ) {
        _list( path, v, Type::_FILE );
    }
}

#endif
