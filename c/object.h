#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_CLOSURE(value)  ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)  ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)  (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)  (((ObjString*)AS_OBJ(value))->chars)


typedef enum {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
  ObjType type;
  bool isMarked;
  struct Obj* next; // Linkedlist of objects to simplify freeing memory.
};

typedef struct {
    Obj obj;
    int arity; // number of parameters
    int upvalueCount;
    Chunk chunk; // bytecode
    ObjString* name;
} ObjFunction;

// bool indicates if function executed correctly, return value returned as args[0].
typedef bool (*NativeFn) (int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function; // pointer to a c function
} ObjNative;

struct ObjString {
    Obj obj; // The Obj struct will be inlined.
    uint32_t hash;
    int length;
    char chars[]; // Flexible array member to inline string in the struct.
};

typedef struct ObjUpvalue {
    Obj obj;
    Value* location; // Pointer to a Value, multiple closures can reference the same variable.
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues; // Array of pointers to upvalues.
    int upvalueCount;
} ObjClosure;

ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
uint32_t hashString(const char* key, int length);
// Takes ownership of the passed in string.
ObjString* makeString(int length, uint32_t hash);
// Does not take ownership of chars it takes.
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif