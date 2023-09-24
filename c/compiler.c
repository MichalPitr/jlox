#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    // ignore further errors if already in panic mode.
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        // Print part of a string, neat.
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    // Does the last parsed token match the expected type?
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static void emitConstant(Value value) {
    addConstant(compilingChunk, value);
}

static void endCompiler() {
    emitReturn();
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after an expression.");
}

static void number() {
    // Simple starts walking from the starting pointer until it encounters a
    // character no belonging to a number.
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary() {
    // operator was already consumer, so retrieve its type from previous.
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    expression();

    // Emit theo operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // This should be unreachable.
    }
}

static void expression() {

}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    // Makes the chunk available throughout the module.
    compilingChunk = chunk;
    advance();
    expression();
    consume(TOKEN_EOF, "Expected end of expression.");
    endCompiler();
    return !parser.hadError;
}
