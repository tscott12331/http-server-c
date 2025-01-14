CC=gcc
CFLAGS=-Wall -Wextra -pedantic-errors

hellomake: main.o 
	$(CC) -o server main.o 
