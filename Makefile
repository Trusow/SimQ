simq-server:
	g++ simq-server.cpp \
	\
	-lcrypto -ldl -pthread -L/usr/lib/ -static -std=c++2a -s -O3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o ./bin/simq-server
