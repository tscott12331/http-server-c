#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "arrays.h"

void initTable(Table* table) {
    table->capacity = 0;
    table->count = 0;
    table->cells = NULL;
}

static int hashString(char* name) {
    // NOT IMPLEMENTED
    int hash;
    for(int i = 0; i < (int)strlen(name); i++) {
        hash += name[i];
        hash = hash ^ 322462468249924;
        hash = hash << 1;
    }

    return hash;
}

static Cell* findCell(Cell* cellList, char* name, int capacity) {
    int hash = hashString(name);
    int index = hash % capacity;
    Cell* current = &cellList[index]; 
    while(current->name != NULL) {
        // spots taken
        current = &cellList[++index];
    }
    return current;
}

static void growTableCapacity(Table* table) {
    int newCapacity = GROW_CAPACITY(table->capacity);
    Cell* newCells = NULL;
    newCells = GROW_ARRAY(newCells, Cell, newCapacity);
    for(int i = 0; i < newCapacity; i++) {
        newCells[i].name = NULL;
        newCells[i].value = NULL;
    }

    for(int i = 0; i < table->capacity; i++) {
        if(table->cells[i].name == NULL) continue;
        char* cellName = table->cells[i].name;
        char* cellValue = table->cells[i].value;
        Cell* newCell = findCell(newCells, cellName, newCapacity);
        newCell->name = cellName;
        newCell->value = cellValue;
    }

    free(table->cells);
    table->capacity = newCapacity;
    table->cells = newCells;
}

bool tableSet(Table* table, char* name, char* value) {
    if(table->count + 1 > table->capacity * LOAD_FACTOR) {
        // need to grow LeTable
        growTableCapacity(table);
    }
    int hash = hashString(name); 
    int index = hash % table->capacity;
    Cell* cell = &table->cells[index];
    while(cell->name != NULL) {
        cell = &table->cells[++index];
    }
    // cell is unused at this point.
    cell->name = name;
    cell->value = value;
    return true;
}

char* tableGet(Table* table, char* name) {
    int hash = hashString(name);
    int index = hash % table->capacity;
    Cell* cell = &table->cells[index];
    while(index < table->capacity && cell != NULL && ((strlen(cell->name) != strlen(name)) || 
            (strcmp(cell->name, name) != 0))) {
       cell = &table->cells[++index]; 
    }
    if(cell == NULL) return NULL;
    return cell->value;
}
