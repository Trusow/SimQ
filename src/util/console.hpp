#ifndef SIMQ_UTIL_CONSOLE
#define SIMQ_UTIL_CONSOLE

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

namespace simq::util {
    class Console {
        private:
            enum Codes {
                KEY_SIGNAL = 3,
                KEY_UP = 65,
                KEY_DOWN = 66,
                KEY_RIGHT = 67,
                KEY_LEFT = 68,
                KEY_ENTER = 10,
                KEY_BACKSPACE = 127,
            };

            const char *_prefix = "test> ";

            void _prev( std::string &line, unsigned int &position );
            void _next( std::string &line, unsigned int &position );

            void _paste( std::string &line, unsigned int &position, char ch );

            bool _isAllowedChar( char ch );

            void _run();

        public:
            Console();
            ~Console();
    };

    Console::Console() {
        struct termios oldt, newt;
        int oldf;
         
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        oldf = fcntl( STDIN_FILENO, F_GETFL, 0 );
        fcntl( STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK );

        _run();
    }

    Console::~Console() {
        struct termios oldt;
        int oldf;
 
        tcgetattr( STDIN_FILENO, &oldt );
        oldt.c_lflag = ( ICANON | ECHO );
        oldf = fcntl( STDIN_FILENO, F_GETFL, 0 );
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
        fcntl( STDIN_FILENO, F_SETFL, oldf & ~O_NONBLOCK );
    }

    bool Console::_isAllowedChar( char ch ) {
        bool isNum = ch >= '0' && ch <= '9';
        bool isLetterD = ch >= 'a' && ch <= 'z';
        bool isLetterU = ch >= 'A' && ch <= 'Z';
        bool isOther = ch == ' ' || ch == '.' || ch == '/';
 
        return isNum || isLetterD || isLetterU || isOther;
    }

    void Console::_run() {
        int ch;
        unsigned int navigation = 0;

        std::string line = "";
        unsigned int position = 0;

        std::chrono::milliseconds ts(50);

        std::cout << _prefix;

        while( true ) {
            ch = getchar();

            if( ch == -1 ) {
                std::this_thread::sleep_for(ts);
            } else if( ch == KEY_SIGNAL ) {
                std::cout << std::endl;
                break;
            } else if( ch == 27 ) {
                if( navigation == 0 ) {
                    navigation++;
                } else {
                    navigation = 0;
                }
            } else if( ch == 91 ) {
                if( navigation == 1 ) {
                    navigation++;
                } else {
                    navigation = 0;
                }
            } else if( ch == KEY_UP && navigation == 2 ) {
                navigation = 0;
            } else if( ch == KEY_DOWN && navigation == 2 ) {
                navigation = 0;
            } else if( ch == KEY_RIGHT && navigation == 2 ) {
                navigation = 0;
                _next( line, position );
            } else if( ch == KEY_LEFT && navigation == 2 ) {
                navigation = 0;
                _prev( line, position );
            } else if( ch == KEY_ENTER ) {
                line = "";
                position = 0;
                std::cout << std::endl;
                std::cout << _prefix;
            } else if( ch == KEY_BACKSPACE ) {
            } else if( _isAllowedChar( ch ) ) {
                _paste( line, position, ch );
            }
        }
    }

    void Console::_next( std::string &line, unsigned int &position ) {
        if( position == line.length() ) {
            return;
        }

        position++;
        std::cout << (char)27 << char(91) << char(67);
    }

    void Console::_prev( std::string &line, unsigned int &position ) {
        if( position == 0 ) {
            return;
        }

        position--;
        std::cout << (char)27 << char(91) << char(68);
    }

    void Console::_paste( std::string &line, unsigned int &position, char ch ) {
        if( position == line.length() ) {
            line += ch;
            position++;
            std::cout << ch;
            return;
        }
    }
}

#endif
