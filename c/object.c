#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    // Place new object at the head of the linked list.
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

// static ObjString* allocateString(char* chars, int length) {
//     ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
//     string->length = length;
//     string->chars = chars;
//     return string;
// }

// Sort of like constructor
ObjString* makeString(int length) {
    ObjString* string = (ObjString*)allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    return string;
}

ObjString* copyString(const char* chars, int length) {
    // Allocate space for string
    ObjString* string = makeString(length);
    // copy string
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            // Neat.
            printf("%s", AS_CSTRING(value));
            break;
    }
}