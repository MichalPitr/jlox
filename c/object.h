#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value))->chars)


typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj* next; // Linkedlist of objects to simplify freeing memory.
};

typedef struct {
    Obj obj;
    int arity; // number of parameters
    Chunk chunk; // bytecode
    ObjString* name;
} ObjFunction;

struct ObjString {
    Obj obj; // The Obj struct will be inlined.
    uint32_t hash;
    int length;
    char chars[]; // Flexible array member to inline string in the struct.
};

ObjFunction* newFunction();
uint32_t hashString(const char* key, int length);
// Takes ownership of the passed in string.
ObjString* makeString(int length, uint32_t hash);
// Does not take ownership of chars it takes.
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif