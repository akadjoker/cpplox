#pragma once
#include "value.h"
#include <functional>
#include <string>
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
    void registerFunction(const std::string &name, int arity, NativeFunction fn);
    NativeFn *getFunction(const std::string &name);
    bool hasFunction(const std::string &name) const;
    void registerBuiltins();

private:
    std::unordered_map<std::string, NativeFn> functions_;
};
