#include <stdio.h>

#include "arrays.h"

void* growArray(void* pointer, int oldCapacity, size_t newSize) {
    /*printf("attempting to realloc to %zu bytes from old capacity %d\n", newSize, oldCapacity);*/
    void* res = realloc(pointer, newSize);    
    if(res == NULL) {
        /*printf("failed to realloc to size %zu\n", newSize);*/
        exit(1);
    }
    /*printf("realloc'd to %zu bytes\n", newSize);*/
    return res;
}
