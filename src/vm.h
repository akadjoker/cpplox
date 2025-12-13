#pragma once
#include "chunk.h"
#include "callframe.h"
#include "native.h"
#include <array>

enum class InterpretResult
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

class VM
{
public:
    static constexpr int STACK_MAX = 256;
    static constexpr int FRAMES_MAX = 64;

    VM();

    InterpretResult interpret(Function *function);
    void registerNative(const std::string &name, int arity, NativeFunction fn);

    Value *getStackTop() { return stackTop_; }

private:
    Value stack_[STACK_MAX];
    Value *stackTop_;

    CallFrame frames_[FRAMES_MAX];
    int frameCount_;

    NativeRegistry natives_;

    bool run();

    void push(Value value);
    Value pop();
    Value peek(int distance);

    bool callNative(const std::string &name, int argCount);

    void runtimeError(const char *format, ...);
    void resetStack();
};
