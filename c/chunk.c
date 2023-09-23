#include <stdlib.h>

#include "chunk.h"
#include "memory.h"



void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lineCount = 0;
    chunk->lineCapacity = 0;
    chunk->lines = NULL;
    // same as &(chunk->constants), hence we get the constants array and dereference to get the pointer to the array.
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    // free memory
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
    // initialize to 0, sets chunk to well-defined empty state.
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    // Can index here! Neat.
    chunk->code[chunk->count] = byte;
    chunk->count++;

    // Are we on the same line?
    if (chunk->lineCount > 0 && chunk->lines[chunk->lineCount -1].line == line) {
        return;
    }

    // Otherwise, Append new LineStart.
    if (chunk->lineCapacity < chunk->lineCount + 1) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(LineStart, chunk->lines, oldCapacity, chunk->lineCapacity);
    }

    // pointer to the next uninitialized linestart in the array
    LineStart* lineStart = &chunk->lines[chunk->lineCount++];
    lineStart->offset = chunk->count - 1;
    lineStart->line = line;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// int: instruction - index of the instruction
int getLine(Chunk* chunk, int instruction) {
    int start = 0;
    int end = chunk->lineCount - 1;

    for (;;) {
        int mid = (end-start) / 2 + start; // Cannot overflow
        LineStart* line = &chunk->lines[mid];
        if (instruction < line->offset) {
            end = mid - 1;
        // Return line if it's the last option OR next-line's first byte has higher offset.
        } else if (mid == chunk->lineCount - 1 || instruction < chunk->lines[mid+1].offset) {
            return line->line;
        } else {
            start = mid + 1;
        }
    }
}
