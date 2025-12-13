#pragma once
#include "chunk.h"
#include "callframe.h"
#include "native.h"
#include <array>
#include <unordered_map>
#include <vector>

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
    ~VM();

    InterpretResult interpret(Function *function);
    void registerNative(const std::string &name, int arity, NativeFunction fn);

    Value *getStackTop() { return stackTop_; }
    uint16_t registerFunction(const std::string &name, Function *func);

    const std::unordered_map<std::string, Value> &getGlobals() const
    {
        return globals_;
    }

    Function *getFunction(const char *name);
    Function *getFunction(uint16_t index);

    // ===== STACK API   =====
    const Value& Peek(int index) ; // -1 = topo, 0 = base
    void Push(Value value);
    Value Pop();

    void PushInt(int n);
    void PushDouble(double d);
    void PushString(const char *s);
    void PushBool(bool b);
    void PushNull();

    int ToInt(int index);
    double ToDouble(int index);
    const char *ToString(int index);
    bool ToBool(int index);

    // ===== STACK INFO =====
    int GetTop();
    void SetTop(int index);
    void Remove(int index);
    void Insert(int index);
    void Replace(int index);
    void Copy(int from, int to);

    // ===== TYPE CHECKING =====
    ValueType GetType(int index);
    bool IsInt(int index);
    bool IsDouble(int index);
    bool IsString(int index);
    bool IsBool(int index);
    bool IsNull(int index);
    bool IsFunction(int index);

    // ===== GLOBALS =====
    void SetGlobal(const char *name);
    void GetGlobal(const char *name);

    // ===== FUNCTIONS =====
    void Call(int argCount, int resultCount);
   

    // ===== DEBUG =====
    void DumpStack();
    void DumpGlobals(); 
    const char *TypeName(ValueType type);

private:
    Value stack_[STACK_MAX];
    Value *stackTop_;

    CallFrame frames_[FRAMES_MAX];
    int frameCount_;

    std::unordered_map<std::string, Value> globals_;

    std::vector<Function *> functions_;
    std::unordered_map<std::string, uint16_t> functionNames_;

    NativeRegistry natives_;

    bool run();
    bool executeUntilReturn(int targetFrameCount) ;

    bool isTruthy(const Value &value);

    void push(Value value);
    Value pop();
    const Value& peek(int distance);

    bool callNative(const std::string &name, int argCount);
    bool callFunction(Function *function, int argCount);

    void runtimeError(const char *format, ...);
    void resetStack();
};
