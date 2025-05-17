CC=clang
INCLUDES = -I. -I/usr/local/include
CFLAGS = -O3 $(INCLUDES)
LDFLAGS = -L/usr/local/lib

.PHONEY: build, run

run: build
	./main $(file)

build: main.o uselibpng.o
	$(CC) $(CFLAGS) $(LDFLAGS) -lpng -lm main.o uselibpng.o -o main

main.o: main.c uselibpng.h
	$(CC) $(CFLAGS) -c main.c

uselibpng.o: uselibpng.c
	$(CC) $(CLFAGS) -c uselibpng.c

clean:
	rm -rf main *.o *.png

