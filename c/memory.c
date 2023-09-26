#include <stdlib.h>
#include <stdio.h>

#include "memory.h"
#include "vm.h"

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

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            // can free since the string is inlined. sizeof assumes the string is 0 len, so we need to add length and '\0'.
            reallocate(object, sizeof(ObjString) + string->length + 1, 0);
            break;
        }
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}