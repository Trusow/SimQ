#ifndef SIMQ_CORE_SERVER_SERVER_MANAGER
#define SIMQ_CORE_SERVER_SERVER_MANAGER

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "callbacks.h"
#include "../../../util/error.h"

namespace simq::core::server::server {
    class Manager {
        private:
            Callbacks *_callbacks = nullptr;

            unsigned short int _port;
            unsigned int _ep;
            unsigned int _sfd;

            const unsigned int USER_EVENTS = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET | EPOLLERR;
            const unsigned int SERVER_EVENTS = EPOLLIN | EPOLLET;
            const unsigned int COUNT_EVENTS = 100;
            const unsigned int COUNT_LISTEN = 500;
            const unsigned int TIMEOUT = 2000;

            unsigned int _createSocket();
            void _bindSocket();
            bool _accept( int &cfd, unsigned int &ip );

        public:
            Manager( unsigned short int port );
            void bindController( Callbacks *callbacks );
            void run();
    };

    Manager::Manager( unsigned short int port ) {
        _port = port;
        _ep = epoll_create1( 0 );
        _sfd = _createSocket();
        _bindSocket();
    }

    void Manager::bindController( Callbacks *callbacks ) {
        _callbacks = callbacks;
    }

    void Manager::run() {
        if( _callbacks == nullptr ) {
            return;
        }

        if( listen( _sfd, COUNT_LISTEN ) == -1 ) {
            throw util::Error::SOCKET;
        }

        struct epoll_event server_event;
        struct epoll_event events[COUNT_EVENTS];
        server_event.events = SERVER_EVENTS;
        server_event.data.fd = _sfd;

        if( epoll_ctl( _ep, EPOLL_CTL_ADD, _sfd, &server_event ) == -1 ) {
            throw util::Error::SOCKET;
        }

        auto lastTS = time( NULL );

        while( true ) {
            int count_events = epoll_wait( _ep, events, COUNT_EVENTS, TIMEOUT );

            if( count_events == -1 ) {
                continue;
            }

            for( unsigned int i = 0; i < count_events; i++ ) {
                auto fd = events[i].data.fd;

                if( fd == -1 ) {
                    continue;
                }

                if( events[i].events & ( EPOLLRDHUP ) || events[i].events & ( EPOLLERR ) ) {
                    _callbacks->disconnect( fd );
                    close( fd );
                } else if( events[i].events & EPOLLIN ) {
                    if( fd == _sfd ) {
                        int cfd;
                        unsigned int ip;
                        if( !_accept( cfd, ip ) ) {
                            break;
                        }
                        _callbacks->connect( cfd, ip );
                    } else {
                        _callbacks->recv( fd );
                    }
                } else if( events[i].events & EPOLLOUT ) {
                    _callbacks->send( fd );
                }
            }

            auto localLastTS = time( NULL );

            if( localLastTS - lastTS > TIMEOUT / 1000 ) {
                lastTS = localLastTS;
                _callbacks->iteration();
            }
        }
    }

    bool Manager::_accept( int &cfd, unsigned int &ip ) {
        struct sockaddr_in addr_client;
        socklen_t size = sizeof( addr_client );
        cfd = ::accept(
            _sfd,
            ( struct sockaddr * )&addr_client,
            &size
        );

        if( cfd == -1 ) {
            return false;
        }

        if( fcntl( cfd, F_SETFL, O_NONBLOCK ) == -1 ) {
            close( cfd );
            return false;
        }

        ip = addr_client.sin_addr.s_addr;

        struct epoll_event ev;
        ev.events = USER_EVENTS;
        ev.data.fd = cfd;


        if( epoll_ctl(
            _ep,
            EPOLL_CTL_ADD,
            cfd,
            &ev
        ) == -1 ) {
            return false;
        }

        auto optval = 1;
        setsockopt( cfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof( optval ) );
        optval = 0;
        setsockopt( cfd, IPPROTO_TCP, TCP_CORK, &optval, sizeof( optval ) );

        return true;
    }

    unsigned int Manager::_createSocket() {
        auto sock = socket( AF_INET, SOCK_STREAM, 0 );

        if( sock == -1 ) {
            throw util::Error::SOCKET;
        }

        auto optval = 1;
        setsockopt( sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof( optval ) );
        setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof( optval ) );

        auto res = fcntl( sock, F_SETFL, O_NONBLOCK );
        if( res == -1 ) {
            throw util::Error::SOCKET;
        }

        return sock;
    }

    void Manager::_bindSocket() {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons( _port );
        addr.sin_addr.s_addr = htonl( INADDR_ANY );

        if( bind( _sfd, ( struct sockaddr * )&addr, sizeof( addr ) ) == -1 ) {
            throw util::Error::SOCKET;
        }
    }
}


#endif
 
