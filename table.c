#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    int index = hash & (capacity - 1);
    printf("findCell: index %d\n", index);
    Cell* current = &cellList[index]; 
    while(current->name != NULL) {
        // spots taken
        index++;
        if(index == capacity) {
            index = index & (capacity - 1);
        }
        current = &cellList[index];
    }
    return current;
}

static void growTableCapacity(Table* table) {
    int newCapacity = GROW_CAPACITY(table->capacity);
    Cell* newCells = (Cell*) malloc(newCapacity * sizeof(Cell)); 
    Cell empty = {
        .name = NULL,
        .value = NULL
    };
    for(int i = 0; i < newCapacity; i++) {
        newCells[i] = empty;
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
    table->cells = NULL;
    table->capacity = newCapacity;
    table->cells = newCells;
}

bool tableSet(Table* table, char* name, char* value) {
    if(table->count + 1 > table->capacity * LOAD_FACTOR) {
        // need to grow LeTable
        printf("growing LeTable\n");
        growTableCapacity(table);
    }
    int hash = hashString(name); 
    int index = hash & (table->capacity - 1);
    printf("index %d, capacity %d\n", index, table->capacity);
    Cell* cell = &table->cells[index];
    printf("at index, cell is %s\n", cell == NULL ? "NULL" : "NOT NULL");
    printf("cell name %s\n", cell->name);
    while(cell != NULL && cell->name != NULL) {
        index++;
        if(index == table->capacity) {
            index = index & (table->capacity - 1);
        }
        cell = &table->cells[index];
    }
    if(cell == NULL) {
        printf("failed to set table for item of name %s\n", name);
        return false;
    }

    // cell is unused at this point.
    printf("setting in table a cell with name '%s' and index '%d'\n", name, index);
    cell->name = name;
    cell->value = value;
    table->count++;
    return true;
}

char* tableGet(Table* table, char* name) {
    int hash = hashString(name);
    int index = hash & (table->capacity - 1);
    Cell* cell = &table->cells[index];
    while(cell != NULL && cell->name != NULL && ((strlen(cell->name) != strlen(name)) || 
            (strcmp(cell->name, name) != 0))) {
        index++;
        if(index == table->capacity) {
            index = index & (table->capacity - 1);
        }
        printf("tableGet: new index %d\n", index);
        cell = &table->cells[index]; 
        printf("tableGet: cell name %s\n", cell == NULL ? "CELL NULL" : cell->name == NULL ? "NAME NULL" : cell->name);
    }
    if(cell == NULL || cell->name == NULL) return NULL;
    /*printf("after searching the table for %s, we found cell with name %s\n", name == NULL ? "NULL" : name, cell->name == NULL ? "NULL" : cell->name);*/
    return cell->value;
}
