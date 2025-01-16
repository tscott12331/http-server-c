#ifndef server_arrays_h
#define server_arrays_h

#define GROW_FACTOR 2
#define GROW_CAPACITY(oldCapacity)                     (oldCapacity == 0 ? 8 : oldCapacity * GROW_FACTOR)
#define GROW_ARRAY(pointer, type, newCapacity)         ((type*)realloc(pointer, newCapacity * sizeof(type)))

#endif
