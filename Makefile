client:
	g++ client.cpp \
	\
	src/tui/button.cpp \
	src/tui/input.cpp \
	src/tui/dialog.cpp \
	src/tui/dialog_auth.cpp \
	src/tui/dialog_groups.cpp \
	src/tui/dialog_settings.cpp \
	src/tui/dialog_groups_test.cpp \
	src/tui/main.cpp \
	\
	-ldl -pthread -L/usr/lib/ -lfinal -lncursesw -static -std=c++2a -s -O3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o ./bin/client
#	-lcrypto -ldl -pthread -L/usr/lib/ -lfinal -lncursesw -static -std=c++2a -s -O3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o ./bin/client
test:
	g++ test.cpp \
	\
	-lcrypto -ldl -pthread -L/usr/lib/ -lfinal -lncursesw -std=c++2a -s -O3 -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -o ./bin/test
