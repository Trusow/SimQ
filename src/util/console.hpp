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

namespace simq::util {
    class Console {
        private:
            enum Mode {
                MODE_NORMAL,
                MODE_PASSWORD,
                MODE_CONFIRM,
            };

            Mode _mode = MODE_NORMAL;

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

            void _pushHistory( std::string &line );
            void _nextHistory( std::string &line );
            void _prevHistory( std::string &line );
            void _startHistory();
 
            std::list<Code> _codes;

            std::string _normalPrefix;
            std::string _confirmPrefix;
            std::string _passwordPrefix;
            std::string _currentPrefix = "test> ";

            void _prevChar( std::string &line, unsigned int &position );
            void _prevWord( std::string &line, unsigned int &position );
            void _nextChar( std::string &line, unsigned int &position );
            void _nextWord( std::string &line, unsigned int &position );

            void _input( std::string &line, unsigned int &position, char ch );
            void _clear( std::string &line, unsigned int &position );
            void _backspace( std::string &line, unsigned int &position );
            void _delete( std::string &line, unsigned int &position );

            bool _isAllowedChar( char ch );

            void _bindCode( KeyCode code, int c, ... );
            void _initCodes();
            KeyCode _getCode( int ch );


        public:
            Console();
            ~Console();
            void run();
            void confirm( const char *str );
            void password( const char *str );
            void setPrefix( const char *str );
    };

    Console::Console() {
        _initCodes();

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

    void Console::_startHistory() {
        _navigationHistory = _history.size();
    }

    bool Console::_isAllowedChar( char ch ) {
        bool isNum = ch >= '0' && ch <= '9';
        bool isLetterD = ch >= 'a' && ch <= 'z';
        bool isLetterU = ch >= 'A' && ch <= 'Z';
        bool isOther = ch == ' ' || ch == '.' || ch == '/';
 
        return isNum || isLetterD || isLetterU || isOther;
    }

    void Console::run() {
        int ch;

        std::string line = "";
        unsigned int position = 0;

        std::chrono::milliseconds ts(50);

        std::cout << _currentPrefix;

        while( true ) {
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

            if( _mode == MODE_NORMAL ) {
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
                } else if( code == KEY_ENTER ) {
                    _startHistory();
                    if( line != "" ) {
                        _pushHistory( line );
                    }
                    line = "";
                    position = 0;
                    std::cout << std::endl;
                    std::cout << _currentPrefix;
                } else if( code == KEY_BACKSPACE ) {
                    _backspace( line, position );
                } else if( code == KEY_DELETE ) {
                    _delete( line, position );
                } else if( code == KEY_NONE && ch >= 32 && ch <= 126 ) {
                    _input( line, position, ch );
                }
            } else if( _mode == MODE_PASSWORD ) {
                if( code == KEY_ENTER ) {
                    _mode = MODE_NORMAL;
                } else {
                    line += (char)ch;
                }
            } else if( _mode == MODE_CONFIRM ) {
                switch( ch ) {
                    case 'Y': case 'y':
                        _mode = MODE_NORMAL;
                        break;
                    case 'N': case 'n':
                        _mode = MODE_NORMAL;
                        break;
                }
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
        std::cout << "\33[2K\r";
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

    void Console::confirm( const char *str ) {
        _confirmPrefix = str;
        _mode = MODE_CONFIRM;
    }

    void Console::password( const char *str ) {
        _passwordPrefix = str;
        _mode = MODE_CONFIRM;
    }

    void Console::setPrefix( const char *str ) {
        _normalPrefix = str;
        _mode = MODE_NORMAL;
    }
}

#endif
