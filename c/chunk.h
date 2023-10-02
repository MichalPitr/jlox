#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL, 
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_POP,
    OP_PRINT,
    OP_JUMP_IF_FALSE,
    OP_JUMP, // followed by 2 bytes that specify a 16bit increment to IP.
    OP_LOOP, // followed by 2 bytes that specify a 16bit decrement to IP.
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_RETURN,
} OpCode;

typedef struct {
    int offset; // byte offset of the first instruction on the line
    int line; // line number
} LineStart;

// Dynamic array
typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    // An array where arr[i] denotes which source line code[i] belongs to.
    int lineCount;
    int lineCapacity;
    LineStart* lines; 
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
int getLine(Chunk* chunk, int instruction);
// int writeConstant(Chunk* chunk, Value value, int line);

#endif