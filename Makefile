CC=gcc
CFLAGS=-Wall -Wextra -pedantic-errors -g
OBJECTS = main.o directory-parser.o table.o

server: $(OBJECTS) 
	$(CC) $(CFLAGS) -o server $(OBJECTS) 
