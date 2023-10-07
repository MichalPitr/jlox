#include <stdlib.h>
#include <stdio.h>

#include "chunk.h"
#include "memory.h"
#include "vm.h"



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
    push(value);
    writeValueArray(&chunk->constants, value);
    pop();
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

// int writeConstant(Chunk* chunk, Value value, int line) {
//     int constantIdx = addConstant(chunk, value);
    
//     if (constantIdx < 256) {
//         writeChunk(chunk, OP_CONSTANT, line);
//         writeChunk(chunk, (uint8_t)constantIdx, line);
//     } else {
//         writeChunk(chunk, OP_CONSTANT_LONG, line);
//         // Since we use 1byte array, we need to split 3 bytes into 3 1-byte writes.
//         // We use little-endian encoding.
//         // Use mask to select the least-significant byte
//         writeChunk(chunk, (uint8_t)(constantIdx & 0xff), line);
//         // shift by 1 byte and repeat
//         writeChunk(chunk, (uint8_t)((constantIdx >> 8) & 0xff), line);
//         // shift by 1 byte and repeat
//         writeChunk(chunk, (uint8_t)((constantIdx >> 16) & 0xff), line);
//     }
//     return constantIdx;
// }
