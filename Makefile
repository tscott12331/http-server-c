CC=gcc
CFLAGS=-Wall -Wextra -pedantic-errors
OBJECTS = main.o directory-parser.o 

server: $(OBJECTS) 
	$(CC) $(CFLAGS) -o server $(OBJECTS) 
