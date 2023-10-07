#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


#include "object.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"
#include "value.h"

VM vm;

// We reuse the args array for passing args and returning value
static bool clockNative(int argCount, Value* args) {
    if (argCount != 0) {
        args[-1] = OBJ_VAL(copyString("Expected 0 arguments.", 22));
        return false;
    }
    args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    return true;
}

static bool errNative(int argCount, Value* args) {
    if (argCount != 0) {
        args[-1] = OBJ_VAL(copyString("Expected 0 arguments.", 22));
        return false;
    }
    args[-1] = OBJ_VAL(copyString("Error!", 6));
    return false;
}

static bool hasFieldNative(int argCount, Value* args) {
    // call fails in these cases
    if (argCount != 2) return false;
    if (!IS_INSTANCE(args[0])) return false;
    if (!IS_STRING(args[1])) return false;

    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value dummy;
    args[-1] = BOOL_VAL(tableGet(&instance->fields, AS_STRING(args[1]), &dummy));
    return true;
}

static bool deleteFieldNative(int argCount, Value* args) {
    // call fails in these cases
    if (argCount != 2) return false;
    if (!IS_INSTANCE(args[0])) return false;
    if (!IS_STRING(args[1])) return false;

    ObjInstance* instance = AS_INSTANCE(args[0]);
    return tableDelete(&instance->fields, AS_STRING(args[1]));
}


static void resetStack() {
    // simply point to the start of the stack. It doesn't matter if the rest of the stack is dirty.
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

// Let's us specify variable number of args.
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code -1;
        int line = getLine(&function->chunk, instruction); // Need to use this since we use compressed line encoding.
        fprintf(stderr, "[line %d] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    
    resetStack();
}

static void defineNative(const char* name, NativeFn function) {
    // pushing to the stack to indicate to GC that we aren't done with the values.
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024; // 1MB

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
    defineNative("err", errNative);
    defineNative("hasField", hasFieldNative);
    defineNative("deleteField", deleteFieldNative);
}



void freeVM() {
    initTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value) {
    // stackTop is a pointer, by dereferencing it, we access the memory location that stackTop points to.
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    // -1 to reserve the function's 0th slot, reserved for methods for later.
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                return true;
            }
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            case OBJ_NATIVE:
                NativeFn native = AS_NATIVE(callee);
                if (native(argCount, vm.stackTop - argCount)) {
                    vm.stackTop -= argCount;
                    return true;
                } else {
                    runtimeError(AS_STRING(vm.stackTop[-argCount-1])->chars);
                    return false;
                }
            default:
                break; // non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    // found upvalue.
    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    // Otherwise make a new upvalue and insert it into the sorted linked list.
    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;
    
    // insert at the head.
    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        // Simply points to its own field. 
        // This lets us reuse the same OP_GET/SET_UPVALUE without change.
        upvalue->location = &upvalue->closed; 
        
        vm.openUpvalues = upvalue->next;
    }
}

/* 
    Value is "falsey" if its null or false, otherwise true
    This makes 0 also true, which feels odd.
*/
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;

    ObjString* result = makeString(length, 0);
    pop();
    pop();
    memcpy(result->chars, a->chars, a->length);
    memcpy(result->chars + a->length, b->chars, b->length);
    result->chars[length] = '\0';
    
    // Check if the concated string is already interned, if so, just return that.
    uint32_t hash = hashString(result->chars, result->length);
    ObjString* interned = tableFindString(&vm.strings, result->chars, length, hash);
    if (interned != NULL) {
        push(OBJ_VAL(interned));
        return;
    }
    
    // otherwise set the hash and add the string to vm.strings table.
    result->hash = hash; 
    tableSet(&vm.strings, result, NIL_VAL);

    push(OBJ_VAL(result));
}

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount-1];
    register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() \
    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_STRING() AS_STRING(READ_CONSTANT())
// do while macro ensures the expanded statements are in the same scope.
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) ||!IS_NUMBER(peek(1))) { \
            frame->ip = ip; \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (false)

    // read, decode, and dispatch bytecode
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            // dereference to access the value at the pointer.
            printValue(*slot);
            printf (" ]");
        }
        printf("\n");
        disassambleInstruction(&frame->closure->function->chunk, 
            (int)(frame->ip - frame->closure->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                // Reads the value from the stack and then pushes it to the top to make
                // it accessible by other instructions.
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    frame->ip = ip;
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                // Get variable name from constant table.
                ObjString* name = READ_STRING();
                // Get value from top of stack and store in hash table.
                tableSet(&vm.globals, name, peek(0));
                // Only pop after value is added to hash set, otherwise it might
                // get garbage collected. Wild.
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                // Var has to be in hashset already, otherwise asignment is invalid. 
                // If key exists doesn't exist, tableSet returns true.
                // If it exists, we simply overwrite it.
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name); // undo setting.
                    frame->ip = ip;
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // pops the instance.
                    push(value);
                    break;
                }

                runtimeError("Undefined property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a+b));
                } else {
                    frame->ip = ip;
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: 
                // We define falsiness of a value.
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    frame->ip = ip;
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                // Modifies value in place.
                *(vm.stackTop - 1) = NUMBER_VAL(-AS_NUMBER(*(vm.stackTop - 1)));
                break;
            }
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) ip += offset;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                frame->ip = ip; // Store back into frame.
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip; // Update after function call finishes.
                break;
            }
            case OP_CLOSURE: {
                // read function and wrap it in a closure, push it on stack.
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        // Stores local in upvalue
                        closure->upvalues[i] =
                            captureUpvalue(frame->slots + index);
                    } else {
                        // Stores upvalue from enclosing in current's upvalues.
                        // Frame referes to enclosing function here.
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                // When function returns we may need to hoist some variables.
                closeUpvalues(frame->slots);
                vm.frameCount--;
                // Return in top-level code. 
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots; // pops function's frame from the stack.
                push(result);
                frame = &vm.frames[vm.frameCount-1];    
                ip = frame->ip;
                break;
            }
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop(); // pop function from stack, again to keep  GC from collecting function.
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}