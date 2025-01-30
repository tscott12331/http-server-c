CC=gcc
CFLAGS=-Wall -Wextra -pedantic-errors -g -fsanitize=address -O3 -fno-omit-frame-pointer
OBJECTS = main.o directory-parser.o table.o arrays.o css.o

server: $(OBJECTS) 
	$(CC) $(CFLAGS) -o server $(OBJECTS) 
