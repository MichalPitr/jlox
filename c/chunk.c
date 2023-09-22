#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    // same as &(chunk->constants), hence we get the constants array and dereference to get the pointer to the array.
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    // free memory
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    // initialize to 0, sets chunk to well-defined empty state.
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    // Can index here! Neat.
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
