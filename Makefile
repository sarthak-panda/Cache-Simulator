CC = g++
CFLAGS = -Wall -Wextra --pedantic -std=c++11 -ggdb3

.PHONY: all clean

all: cacheSim

cacheSim: main.o
	$(CC) $(CFLAGS) -o cacheSim main.o -lstdc++

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -f cacheSim main.o
