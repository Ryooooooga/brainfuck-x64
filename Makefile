CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Werror -g
LDFLAGS  :=

.PHONY: all hello_world clean

all: test

test: test.cpp bfx64.hpp

hello_world: test
	./test > hello_world.s
	gcc -c hello_world.s
	gcc -o hello_world hello_world.s
	./hello_world

clean:
	${RM} test hello_world *.s
