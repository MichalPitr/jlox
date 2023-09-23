#include <stdlib.h>
#include <stdio.h>

#include "memory.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }
    
    void* result = realloc(pointer, newSize);
    // Allocating memory failed - maybe no free memory?
    if (result == NULL) {
        fprintf(stderr, "Reallocating failed!\n");
        exit(1);
    }
    return result;
}