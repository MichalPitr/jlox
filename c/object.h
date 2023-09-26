#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value))->chars)


typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj* next; // Linkedlist of objects to simplify freeing memory.
};

struct ObjString {
    Obj obj; // The Obj struct will be inlined.
    int length;
    char chars[]; // Flexible array member to inline string in the struct.
};

// Takes ownership of the passed in string.
ObjString* makeString(int length);
// Does not take ownership of chars it takes.
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif