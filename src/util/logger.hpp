#ifndef SIMQ_UTIL_LOGGER
#define SIMQ_UTIL_LOGGER

#include <mutex>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include <string>

namespace simq::util {
    class Logger {
        private:
            inline static std::mutex _m;
            static void _printTime();
            static void _print(
                std::list<std::string> &list,
                char separator,
                const char *color = nullptr,
                bool isRaw = false
            );
        public:
            static void printDanger( std::list<std::string> &list, char separator = '\t' );
            static void print( std::list<std::string> &list, char separator = '\t' );
            static void printRaw( std::list<std::string> &list, char separator = '\t' );
    };

    void Logger::_printTime() {
        time_t _time;
        char buffer[20]{};

        time( &_time );
        strftime ( buffer, 20, "%Y-%m-%d %H:%M:%S", localtime( &_time ) );
 
        std::cout << buffer;
    }

    void Logger::_print(
        std::list<std::string> &list,
        char separator,
        const char *color,
        bool isRaw
    ) {
        std::lock_guard<std::mutex> lock( _m );
        auto _printColor = color != nullptr && isatty( fileno( stdout ) );

        if( _printColor ) {
            std::cout << "\x1b[" << color;
        }

        if( !isRaw ) {
            _printTime();
            std::cout << separator;
        }

        for( auto it = list.begin(); it != list.end(); it++ ) {
            std::cout << *it << separator;
        }

        if( _printColor ) {
            std::cout << "\x1b[0m";
        }

        std::cout << std::endl;

    }

    void Logger::printDanger( std::list<std::string> &list, char separator ) {
        _print( list, separator, "91m" );
    }

    void Logger::print( std::list<std::string> &list, char separator ) {
        _print( list, separator );
    }

    void Logger::printRaw( std::list<std::string> &list, char separator ) {
        _print( list, separator, nullptr, true );
    }
}
 
#endif
