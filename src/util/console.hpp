#ifndef SIMQ_UTIL_CONSOLE
#define SIMQ_UTIL_CONSOLE

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <list>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sstream>
#include "console_callbacks.h"

namespace simq::util {
    class Console {
        private:
            enum Mode {
                MODE_NORMAL,
                MODE_PASSWORD,
                MODE_CONFIRM,
            };

            Mode _mode = MODE_NORMAL;

            ConsoleCallbacks *_cb = nullptr;

            enum KeyCode {
                KEY_PROGRESS,
                KEY_NONE,
             
                KEY_KILL,
                KEY_LEFT,
                KEY_RIGHT,
                KEY_UP,
                KEY_DOWN,
                KEY_CTRL_LEFT,
                KEY_CTRL_RIGHT,
                KEY_CTRL_UP,
                KEY_CTRL_DOWN,
                KEY_BACKSPACE,
                KEY_DELETE,
                KEY_ENTER,
                KEY_F,
            };
 
            struct Code {
                std::vector<unsigned int> sequense;
                unsigned int step;
                KeyCode code;
            };

            std::vector<std::string> _history;
            unsigned int _navigationHistory = 0;

            const char *_margin = "    ";

            void _pushHistory( std::string &line );
            void _nextHistory( std::string &line );
            void _prevHistory( std::string &line );
            void _toBeginHistory();

            bool _isExit = false;
 
            std::list<Code> _codes;

            std::string _normalPrefix = "> ";
            std::string _currentPrefix;

            void _prevChar( std::string &line, unsigned int &position );
            void _prevWord( std::string &line, unsigned int &position );
            void _nextChar( std::string &line, unsigned int &position );
            void _nextWord( std::string &line, unsigned int &position );

            void _input( std::string &line, unsigned int &position, char ch );
            void _clear( std::string &line, unsigned int &position );
            void _backspace( std::string &line, unsigned int &position );
            void _delete( std::string &line, unsigned int &position );

            void _inputNormal( std::string &line, unsigned int &position, KeyCode code, int ch );
            void _inputPassword( std::string &line, unsigned int &position, KeyCode code, int ch );
            void _inputConfirm( std::string &line, unsigned int &position, KeyCode code, int ch );

            void _revertToNormalMode();

            void _bindCode( KeyCode code, int c, ... );
            void _initCodes();
            KeyCode _getCode( int ch );

            unsigned int _getTerminalWidth();
            void _getSplitString(
                const char *text,
                std::vector<std::string> &list,
                char separator,
                bool skipEmpty = false
            );

        public:
            Console( ConsoleCallbacks *cb );
            ~Console();
            void run();

            void confirm( const char *prefix );
            void getPassword( const char *prefix );
            void setPrefix( const char *prefix );

            void printText( const char *text );
            void printSuccess( const char *text );
            void printWarning( const char *text );
            void printDanger( const char *text );

            void printList( std::vector<std::string> &list, const char *text = nullptr );

            void printPrefix( bool newLine = false );

            void exit();
    };

    Console::Console( ConsoleCallbacks *cb ) {
        _cb = cb;
        _initCodes();
        _currentPrefix = _normalPrefix;

        struct termios oldt, newt;
        int oldf;
         
        tcgetattr( STDIN_FILENO, &oldt );
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );
        oldf = fcntl( STDIN_FILENO, F_GETFL, 0 );
        fcntl( STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK );
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

    void Console::_pushHistory( std::string &line ) {
        if( !_history.empty() && _history[_history.size()-1] == line ) {
            return;
        }

        _history.push_back( line );
        _navigationHistory++;
    }

    void Console::_nextHistory( std::string &line ) {
        if( _navigationHistory == _history.size() ) {
            return;
        }

        line = _history[_navigationHistory];

        _navigationHistory++;
    }

    void Console::_prevHistory( std::string &line ) {
        if( _history.empty() || _navigationHistory == 0 ) {
            return;
        }

        line = _history[_navigationHistory-1];

        _navigationHistory--;
    }

    void Console::_toBeginHistory() {
        _navigationHistory = _history.size();
    }

    void Console::_revertToNormalMode() {
        _currentPrefix = _normalPrefix;
        _mode = MODE_NORMAL;
    }

    void Console::_inputNormal( std::string &line, unsigned int &position, KeyCode code, int ch ) {
        if( code == KEY_LEFT ) {
            _prevChar( line, position );
        } else if( code == KEY_CTRL_LEFT ) {
            _prevWord( line, position );
        } else if( code == KEY_RIGHT ) {
            _nextChar( line, position );
        } else if( code == KEY_CTRL_RIGHT ) {
            _nextWord( line, position );
        } else if( code == KEY_DOWN ) {
            _clear( line, position );
            _nextHistory( line );
            position = line.length();
            std::cout << line;
        } else if( code == KEY_UP ) {
            _clear( line, position );
            _prevHistory( line );
            position = line.length();
            std::cout << line;
        } else if( code == KEY_CTRL_UP ) {
            std::cout << _getTerminalWidth() << std::endl;
        } else if( code == KEY_ENTER ) {
            _toBeginHistory();
            if( line != "" ) {
                _pushHistory( line );
            }

            std::vector<std::string> list;
            _getSplitString( line.c_str(), list, ' ', true );

            line = "";
            position = 0;

            if( !list.empty() ) {
                _cb->input( list );
            } else {
                std::cout << std::endl;
                std::cout << _currentPrefix;
            }
        } else if( code == KEY_BACKSPACE ) {
            _backspace( line, position );
        } else if( code == KEY_DELETE ) {
            _delete( line, position );
        } else if( code == KEY_NONE && ch >= 32 && ch <= 126 ) {
            _input( line, position, ch );
        }
    }

    void Console::_inputPassword( std::string &line, unsigned int &position, KeyCode code, int ch ) {
        if( code == KEY_ENTER ) {
            _revertToNormalMode();

            _cb->inputPassword( line.c_str() );
            line = "";
            position = 0;
        } else {
            line += (char)ch;
        }
    }

    void Console::_inputConfirm( std::string &line, unsigned int &position, KeyCode code, int ch ) {
        switch( ch ) {
            case 'Y': case 'y':
                _revertToNormalMode();
                _cb->confirm( true );
                break;
            case 'N': case 'n':
                _revertToNormalMode();
                _cb->confirm( false );
                break;
        }
    }

    void Console::run() {
        int ch;

        std::string line = "";
        unsigned int position = 0;

        std::chrono::milliseconds ts(50);

        while( true ) {
            if( _isExit ) {
                std::cout << std::endl;
                break;
            }

            ch = getchar();

            if( ch == -1 ) {
                std::this_thread::sleep_for(ts);
                continue;
            }

            auto code = _getCode( ch );

            if( code == KEY_KILL ) {
                std::cout << std::endl;
                break;
            }

            switch( _mode ) {
                case MODE_NORMAL:
                    _inputNormal( line, position, code, ch );
                    break;
                case MODE_PASSWORD:
                    _inputPassword( line, position, code, ch );
                    break;
                case MODE_CONFIRM:
                    _inputConfirm( line, position, code, ch );
                    break;
            }
        }
    }

    void Console::_nextChar( std::string &line, unsigned int &position ) {
        if( position == line.length() ) {
            return;
        }

        position++;
        std::cout << (char)27 << char(91) << char(67);
    }

    void Console::_prevChar( std::string &line, unsigned int &position ) {
        if( position == 0 ) {
            return;
        }

        position--;
        std::cout << (char)27 << char(91) << char(68);
    }

    void Console::_prevWord( std::string &line, unsigned int &position ) {
        bool isSpace = position == line.length();
        bool isBreak = false;

        if( position == line.length() ) {
            _prevChar( line, position );
        }

        if( line[position] != ' ' ) {
            _prevChar( line, position );
        }

        for( unsigned int i = position; i > 0; i-- ) {
            if( line[i] == ' ' ) {
                if( isBreak ) {
                    _nextChar( line, position );
                    break;
                }
                isSpace = true;
            } else if( isSpace || i == position ) {
                isBreak = true;
            }
            _prevChar( line, position );
        }
    }

    void Console::_nextWord( std::string &line, unsigned int &position ) {
        bool isSpace = position == 0;
        bool isBreak = false;

        for( unsigned int i = position; i < line.length(); i++ ) {
            if( line[i] == ' ' ) {
                if( isBreak ) {
                    break;
                }
                isSpace = true;
            } else if ( isSpace || i == position ) {
                isBreak = true;
            }
            _nextChar( line, position );
        }
    }

    void Console::_backspace( std::string &line, unsigned int &position ) {
        unsigned int origPosition = position;
        if( position == 0 ) {
            return;
        }

        auto l = line.length();
        char *str = new char[l]{};
        memcpy( &str[0], line.c_str(), position-1 );
        memcpy( &str[position-1], &line.c_str()[position], l-position );

        std::string dline = str;
        delete[] str;

        _clear( line, position );
        line = dline;
        std::cout << line;
        position = line.length();

        origPosition -= 2;
        for( int i = line.length() - 1; i >= 0; i-- ) {
            if( i == origPosition ) {
                break;
            }
            _prevChar( line, position );
        }
    }

    void Console::_delete( std::string &line, unsigned int &position ) {
        unsigned int origPosition = position;
        if( position == line.length() ) {
            return;
        }

        auto l = line.length();
        char *str = new char[l]{};
        memcpy( &str[0], line.c_str(), position );
        memcpy( &str[position], &line.c_str()[position+1], l-position-1 );

        std::string dline = str;
        delete[] str;

        _clear( line, position );
        line = dline;
        std::cout << line;
        position = line.length();

        origPosition--;
        for( int i = line.length() - 1; i >= 0; i-- ) {
            if( i == origPosition ) {
                break;
            }
            _prevChar( line, position );
        }
    }

    void Console::_input( std::string &line, unsigned int &position, char ch ) {
        auto l = line.length();
        unsigned int origPosition = position;
        char *str = new char[l+2]{};
        str[position] = ch;
        memcpy( &str[0], line.c_str(), position );
        memcpy( &str[position+1], &line.c_str()[position], l-position );

        std::string dline = str;
        delete[] str;

        _clear( line, position );
        line = dline;
        std::cout << line;
        position = line.length();

        for( int i = line.length() - 1; i >= 0; i-- ) {
            if( i == origPosition ) {
                break;
            }
            _prevChar( line, position );
        }
    }

    void Console::_clear( std::string &line, unsigned int &position ) {
        auto width = _getTerminalWidth();
        auto l = line.length() + _currentPrefix.length();
        unsigned int count = l / width;

        std::cout << "\x1b[2K\x1b[0G";
        for( int i = 0; i < count; i++ ) {
            std::cout << "\x1b[F\x1b[2K\x1b[0G";
        }

        std::cout << _currentPrefix;
        line = "";
        position = 0;
    }

    void Console::_bindCode( KeyCode code, int c, ... ) {
        Code _code{};
        _code.code = code;
        _code.sequense.push_back( c );
        va_list args;
        va_start( args, c );
 
        while( c-- ) {
            auto co = va_arg( args, int );
            if( co == 0 ) {
                break;
            }
            _code.sequense.push_back( co );
        }
 
        _codes.push_back( _code );
    }

    void Console::_initCodes() {
        _bindCode( KEY_KILL, 3, 0 );

        _bindCode( KEY_LEFT, 27, 91, 68, 0 );
        _bindCode( KEY_RIGHT, 27, 91, 67, 0 );
        _bindCode( KEY_UP, 27, 91, 65, 0 );
        _bindCode( KEY_DOWN, 27, 91, 66, 0 );
        _bindCode( KEY_CTRL_LEFT, 27, 91, 49, 59, 53, 68, 0 );
        _bindCode( KEY_CTRL_RIGHT, 27, 91, 49, 59, 53, 67, 0 );
        _bindCode( KEY_CTRL_UP, 27, 91, 49, 59, 53, 65, 0 );
        _bindCode( KEY_CTRL_DOWN, 27, 91, 49, 59, 53, 66, 0 );
        _bindCode( KEY_BACKSPACE, 127, 0 );
        _bindCode( KEY_ENTER, 10, 0 );
        _bindCode( KEY_DELETE, 27, 91, 51, 126, 0 );

        _bindCode( KEY_F, 27, 79, 80, 0 );
        _bindCode( KEY_F, 27, 79, 81, 0 );
        _bindCode( KEY_F, 27, 79, 82, 0 );
        _bindCode( KEY_F, 27, 79, 83, 0 );
        _bindCode( KEY_F, 27, 91, 49, 53, 126, 0 );
        _bindCode( KEY_F, 27, 91, 49, 55, 126, 0 );
        _bindCode( KEY_F, 27, 91, 49, 56, 126, 0 );
    }

    Console::KeyCode Console::_getCode( int ch ) {
        KeyCode code = KEY_NONE;
        for( auto it = _codes.begin(); it != _codes.end(); it++ ) {
            if( (*it).sequense[(*it).step] == ch ) {
                (*it).step++;
                code = KEY_PROGRESS;
                if( (*it).step == (*it).sequense.size() ) {
                    code = (*it).code;
                    break;
                }
            } else {
                (*it).step = 0;
            }
        }
     
        if( code != KEY_NONE && code != KEY_PROGRESS ) {
            for( auto it = _codes.begin(); it != _codes.end(); it++ ) {
                (*it).step = 0;
            }
        }
     
        return code;
    }

    void Console::confirm( const char *prefix ) {
        _currentPrefix = prefix;
        _currentPrefix += "\n\nPress Y(y)/N(n) to continue";
        _mode = MODE_CONFIRM;
        std::cout << std::endl << "\x1b[36m" << _currentPrefix << "\x1b[0m";
    }

    void Console::getPassword( const char *prefix ) {
        _currentPrefix = prefix;
        _mode = MODE_PASSWORD;
        std::cout << std::endl << _currentPrefix;
    }

    void Console::setPrefix( const char *prefix ) {
        _normalPrefix = prefix;
        _currentPrefix = prefix;
        _mode = MODE_NORMAL;
    }

    unsigned int Console::_getTerminalWidth() {
        winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_col;
    }

    void Console::printText( const char *text ) {
        // XXX No UTF-8!!!

        std::vector<std::string> list;
        _getSplitString( text, list, '\n' );

        std::cout << std::endl;

        for( unsigned int i = 0; i < list.size(); i++ ) {
            std::string str = list[i];

            auto l = str.length();
            auto width = _getTerminalWidth() - strlen( _margin ) * 2;
            unsigned int count = l / width;

            if( l - count * width != 0 ) {
                count++;
            }

            std::cout << std::endl;
            unsigned int offset = 0;

            for( unsigned int i = 0; i < count; i++ ) {
                std::cout << _margin;
                std::cout << str.substr( offset, width );
                offset += width;
                std::cout << _margin << std::endl;
            }
        }

        std::cout << std::endl;
    }

    void Console::printSuccess( const char *text ) {
        std::cout << "\x1b[32m";
        printText( text );
        std::cout << "\x1b[0m";
    }

    void Console::printWarning( const char *text ) {
        std::cout << "\x1b[33m";
        printText( text );
        std::cout << "\x1b[0m";
    }

    void Console::printDanger( const char *text ) {
        std::cout << "\x1b[31m";
        printText( text );
        std::cout << "\x1b[0m";
    }

    void Console::printList( std::vector<std::string> &list, const char *text ) {
        std::cout << std::endl;
        std::cout << std::endl;

        if( text == nullptr ) {
            for( unsigned int i = 0; i < list.size(); i++ ) {
                std::cout << _margin << list[i] << std::endl;
            }
        } else {
            for( unsigned int i = 0; i < list.size(); i++ ) {
                auto offset = list[i].find( text );
                if( offset == std::string::npos ) {
                    continue;
                }

                auto l = strlen( text );
                unsigned int count = 0;
                bool isSearch = false;
                std::cout << _margin;
                for( unsigned int c = 0; c < list[i].length(); c++ ) {
                    if( c == offset ) {
                        isSearch = true;
                        std::cout << "\x1b[33m";
                    }
                    if( isSearch ) {
                        count++;
                    }
                    std::cout << list[i][c];
                    if( l == count ) {
                        isSearch = false;
                        std::cout << "\x1b[0m";
                    }
                }
                std::cout << std::endl;
            }
        }

        std::cout << std::endl;
    }

    void Console::exit() {
        _isExit = true;
    }

    void Console::_getSplitString(
        const char *text,
        std::vector<std::string> &list,
        char separator,
        bool skipEmpty
    ) {
        std::stringstream stream( text );
        std::string line;

        while( std::getline( stream, line, separator ) ) {
            if( skipEmpty && !line.empty() || !skipEmpty ) {
                list.push_back( line );
            }
        }
    }

    void Console::printPrefix( bool newLine ) {
        if( newLine ) {
            std::cout << std::endl;
        }

        std::cout << _currentPrefix;
    }
}

#endif
