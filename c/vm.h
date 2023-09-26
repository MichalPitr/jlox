#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip; // pointer to a byte, pointer dereference is faster than array indexing!
    Value stack[STACK_MAX]; // defined inline, i.e. contiguously within the struct! 
    Value* stackTop;
    Obj* objects; // LinkedList of heap-allocated objets.
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