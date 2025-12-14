#pragma once
#include "value.h"
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

class VM;

using NativeFunction = std::function<Value(VM *, int, Value *)>;

struct NativeFn
{
    std::string name;
    int arity;
    NativeFunction function;

    NativeFn(const std::string &n, int a, NativeFunction fn)
        : name(n), arity(a), function(fn) {}
};

class NativeRegistry
{
public:
    uint16_t registerFunction(const char* name, int arity, NativeFunction fn);
    NativeFn *getFunction(const std::string &name);
    NativeFn *getFunction(uint16_t index);

    bool hasFunction(const char* name) const;
    void registerBuiltins(VM *vm);

private:
    std::unordered_map<std::string, NativeFn> functions_;
    std::vector<NativeFn> builtins_;
};
