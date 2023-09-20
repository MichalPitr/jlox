package com.craftinginterpreters.lox;

public class Return extends RuntimeException {
    final Object value;

    Return(Object value) {
        // Disable unnecessary exception stuff so that returns are faster
        super(null, null, false, false);
        this.value = value;
    }
}
