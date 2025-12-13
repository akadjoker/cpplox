#include "native.h"
#include "vm.h"
#include <cstdio>
#include <cmath>
#include <ctime>

void NativeRegistry::registerFunction(const std::string &name, int arity, NativeFunction fn)
{
    functions_.emplace(name, NativeFn(name, arity, fn));
}

NativeFn *NativeRegistry::getFunction(const std::string &name)
{
    auto it = functions_.find(name);
    if (it != functions_.end())
    {
        return &it->second;
    }
    return nullptr;
}

bool NativeRegistry::hasFunction(const std::string &name) const
{
    return functions_.find(name) != functions_.end();
}

// Built-in functions
static Value nativeClock(VM *vm, int argCount, Value *args)
{
    (void)vm;
    (void)argCount;
    (void)args;
    return Value::makeDouble((double)clock() / CLOCKS_PER_SEC);
}

static Value nativePrint(VM *vm, int argCount, Value *args)
{
    (void)vm;
    for (int i = 0; i < argCount; i++)
    {
        printValue(args[i]);
        if (i < argCount - 1)
            printf(" ");
    }
    printf("\n");
    return Value::makeNull();
}

static Value nativeSqrt(VM *vm, int argCount, Value *args)
{
    (void)vm;
    if (argCount != 1)
    {
        fprintf(stderr, "sqrt() expects 1 argument\n");
        return Value::makeNull();
    }

    if (args[0].isInt())
    {
        return Value::makeDouble(std::sqrt(args[0].asInt()));
    }
    else if (args[0].isDouble())
    {
        return Value::makeDouble(std::sqrt(args[0].asDouble()));
    }

    fprintf(stderr, "sqrt() expects a number\n");
    return Value::makeNull();
}

static Value nativeAbs(VM *vm, int argCount, Value *args)
{
    (void)vm;
    if (argCount != 1)
    {
        fprintf(stderr, "abs() expects 1 argument\n");
        return Value::makeNull();
    }

    if (args[0].isInt())
    {
        return Value::makeInt(std::abs(args[0].asInt()));
    }
    else if (args[0].isDouble())
    {
        return Value::makeDouble(std::fabs(args[0].asDouble()));
    }

    fprintf(stderr, "abs() expects a number\n");
    return Value::makeNull();
}

static Value nativePow(VM *vm, int argCount, Value *args)
{
    (void)vm;
    if (argCount != 2)
    {
        fprintf(stderr, "pow() expects 2 arguments\n");
        return Value::makeNull();
    }

    double base = args[0].isInt() ? args[0].asInt() : args[0].asDouble();
    double exp = args[1].isInt() ? args[1].asInt() : args[1].asDouble();

    return Value::makeDouble(std::pow(base, exp));
}

static Value nativeStr(VM *vm, int argCount, Value *args)
{
    (void)vm;
    if (argCount != 1)
    {
        fprintf(stderr, "str() expects 1 argument\n");
        return Value::makeNull();
    }

    std::string result = valueToString(args[0]);
    return Value::makeString(result.c_str());
}

static Value nativeLen(VM *vm, int argCount, Value *args)
{
    (void)vm;
    if (argCount != 1)
    {
        fprintf(stderr, "len() expects 1 argument\n");
        return Value::makeNull();
    }

    if (!args[0].isString())
    {
        fprintf(stderr, "len() expects a string\n");
        return Value::makeNull();
    }

    return Value::makeInt(args[0].asString()->size());
}

void NativeRegistry::registerBuiltins()
{
    registerFunction("clock", 0, nativeClock);
    registerFunction("print", -1, nativePrint);
    registerFunction("sqrt", 1, nativeSqrt);
    registerFunction("abs", 1, nativeAbs);
    registerFunction("pow", 2, nativePow);
    registerFunction("str", 1, nativeStr);
    registerFunction("len", 1, nativeLen);
}
