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
#include <iostream>
#include "error.h"

namespace simq::util {
    class FS {
        public:
            enum SortDir {
                SortAsc,
                SortDesc,
            };

            enum SortType {
                SortByName,
                SortByDate,
            };
        private:
        enum Type {
            _FILE,
            _DIR,
        };

        struct ListItem {
            std::string name;
            unsigned int time;
            SortType sortType;
            SortDir sortDir;

            bool operator<(const ListItem& a) const {
                if( sortType == SortByName ) {
                    if( sortDir == SortAsc ) {
                        return name < a.name;
                    } else {
                        return name > a.name;
                    }
                } else {
                    if( sortDir == SortAsc ) {
                        return time < a.time;
                    } else {
                        return time > a.time;
                    }
                }
            }
        };

        static void _list( const char *path, std::vector<std::string> &v, Type t, SortDir sortDir, SortType sortType );
        public:
        static bool dirExists( const char *path );
        static bool fileExists( const char *path );
        static bool createDir( const char *path );
        static void removeDir( const char *path );
        static bool removeFile( const char *path );
        static void dirs( const char *path, std::vector<std::string> &v, SortDir sortDir = SortAsc, SortType = SortByName );
        static void files( const char *path, std::vector<std::string> &v, SortDir sortDir = SortAsc, SortType = SortByName );
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

    void FS::_list( const char *path, std::vector<std::string> &v, Type t, SortDir sortDir, SortType sortType ) {
        auto *dir = opendir( path );

        if( dir == nullptr ) {
            throw util::Error::FS_ERROR;
        }
     
        dirent *pDirent;

        std::vector<ListItem> list;

        while (( pDirent = readdir( dir ) ) != nullptr) {
            bool isFile = t == Type::_FILE && pDirent->d_type == DT_REG;
            bool isDir = t == Type::_DIR && pDirent->d_type == DT_DIR;
            if(  isFile || isDir ) {
                std::string fPath = path;
                fPath += "/";
                fPath += pDirent->d_name;

                struct stat st;
                if( stat( fPath.c_str(), &st ) != 0 ) {
                    closedir( dir );
                    throw util::Error::FS_ERROR;
                }

                ListItem item;
                item.name = pDirent->d_name;
                item.time = st.st_ctime;
                item.sortType = sortType;
                item.sortDir = sortDir;
                list.push_back( item );
            }
        }

        closedir( dir );

        std::sort( list.begin(), list.end() );

        for( unsigned int i = 0; i < list.size(); i++ ) {
            v.push_back( list[i].name );
        }
    }

    void FS::dirs( const char *path, std::vector<std::string> &v, SortDir sortDir, SortType sortType ) {
        _list( path, v, Type::_DIR, sortDir, sortType );
    }

    void FS::files( const char *path, std::vector<std::string> &v, SortDir sortDir, SortType sortType ) {
        _list( path, v, Type::_FILE, sortDir, sortType );
    }
}

#endif
