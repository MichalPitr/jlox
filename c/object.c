#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
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

ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

// static ObjString* allocateString(char* chars, int length) {
//     ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
//     string->length = length;
//     string->chars = chars;
//     return string;
// }

// FNV-1a hash algorithm.
uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i]; // xor
        hash *= 16777619; // scatter data around
    }
}

// Sort of like constructor
ObjString* makeString(int length, uint32_t hash) {
    ObjString* string = (ObjString*)allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->hash = hash;
    return string;
}

ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    // When copying a string, if the exact string already exists somewhere,
    // just return that one and don't make a new one.
    ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;
    
    // Allocate space for string
    ObjString* string = makeString(length, hash);
    // copy string
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    tableSet(&vm.strings, string, NIL_VAL);

    return string;
}

void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            // Neat.
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_FUNCTION:
            // Neat.
            printFunction(AS_FUNCTION(value));
            break;
    }
}