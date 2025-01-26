#ifndef server_arrays_h
#define server_arrays_h

#include <stdlib.h>

#define GROW_FACTOR 2
#define GROW_CAPACITY(oldCapacity)                     ((oldCapacity) < 8 ? 8 : (oldCapacity) * GROW_FACTOR)
/*#define GROW_ARRAY(pointer, type, newCapacity)         ((type*)realloc(pointer, newCapacity * sizeof(type)))*/
#define GROW_ARRAY(pointer, type, oldCapacity, newCapacity)     ((type*)growArray(pointer, oldCapacity, newCapacity * sizeof(type)))

void* growArray(void* pointer, int oldCapacity, size_t newSize);

#endif
