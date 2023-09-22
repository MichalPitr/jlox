package com.craftinginterpreters.lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    private final Map<String, LoxFunction> methods;
    private LoxClass superclass;

    LoxClass(LoxClass metaclass, String name, LoxClass superclass, Map<String,  LoxFunction> methods) {
        super(metaclass);
        this.name = name;
        this.methods = methods;
        this.superclass = superclass;
    }

    LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        if (superclass != null) {
            return superclass.findMethod(name);
        }

        return null;
    }

    @Override
    public String toString() {
        return name;
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod("init");
        if (initializer == null) return 0;
        return initializer.arity();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod("init");
        if (initializer != null) {
            // Gives initializer access to 'this'.
            initializer.bind(instance).call(interpreter, arguments);
        }
        return instance;
    }
}
