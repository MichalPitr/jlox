#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)


typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

    Value stack[STACK_MAX]; // defined inline, i.e. contiguously within the struct! 
    Value* stackTop;
    Table globals; // hashmap of global variables.
    Table strings; // hashmap of strings, used to map "equal" strings.
    ObjString* initString;
    ObjUpvalue* openUpvalues;
    
    size_t bytesAllocated;
    size_t nextGC;
    Obj* objects; // LinkedList of heap-allocated objets.
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

// Makes the variable visible in other modules.
extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif