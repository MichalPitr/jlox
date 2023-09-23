#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    int offset;
    int line;
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

#endif