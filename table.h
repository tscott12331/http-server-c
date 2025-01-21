#ifndef server_table_h
#define server_table_h

#include <stdbool.h>

#define LOAD_FACTOR 0.75f

typedef struct {
    char* name;
    char* value; // html string in our case
} Cell;

typedef struct {
    int count;
    int capacity;
    Cell* cells;
} Table;

void initTable(Table* table);
char* tableGet(Table* table, char* name);
bool tableSet(Table* table, char* name, char* value);

#endif
