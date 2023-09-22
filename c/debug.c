#include <stdio.h>

#include "debug.h"
#include "value.h"


void disassambleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassambleInstruction(chunk, offset);
    }
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    // We store (and retrieve) the index of the constant in code!
    uint8_t constantIdx = chunk->code[offset+1];

    printf("%-16s %4d '", name, constantIdx);
    // Access the constant in constants.
    printValue(chunk->constants.values[constantIdx]);
    printf("'\n");
    return offset + 2;
}

int disassambleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

