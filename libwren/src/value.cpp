#include "value.h"
#include "stringpool.h"
#include <cstdio>
#include "value.h"
#include <cstdio>

Value::Value() : type(VAL_NULL)
{
    as.integer = 0;
}

    

Value Value::makeNull()
{
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value Value::makeBool(bool b)
{
    Value v;
    v.type = VAL_BOOL;
    v.as.boolean = b;
    return v;
}

Value Value::makeInt(int i)
{
    Value v;
    v.type = VAL_INT;
    v.as.integer = i;
    return v;
}

Value Value::makeDouble(double d)
{
    Value v;
    v.type = VAL_DOUBLE;
    v.as.number = d;
    return v;
}

Value Value::makeFloat(float f)
{
    Value v;
    v.type = VAL_DOUBLE;
    v.as.number = f;
    return v;
}

Value Value::makeString(const char *str)
{
    Value v;
    v.type = VAL_STRING;
    v.as.string  = StringPool::instance().intern(str);
    return v;
}

Value Value::makeString(const std::string &str)
{
    Value v;
    v.type = VAL_STRING;
    v.as.string = StringPool::instance().intern(str);
    return v;
}

Value Value::makeFunction(int idx)
{
    Value v;
    v.type = VAL_FUNCTION;
    v.as.functionIdx = idx;
    return v;
}

Value Value::makeNative(int idx)
{ 
    Value v;
    v.type = VAL_NATIVE;
    v.as.nativeIdx = idx;
    return v;
}

Value Value::makeProcess(int idx)
{
    Value v;
    v.type = VAL_PROCESS;
    v.as.processIdx = idx;
    return v;
}

bool Value::asBool() const { return as.boolean; }
int Value::asInt() const { return as.integer; }
double Value::asDouble() const { return as.number; }
float Value::asFloat() const { return (float)as.number; }
const char *Value::asString() const { return as.string; }
int Value::asFunctionIdx() const { return as.functionIdx; }
int Value::asNativeIdx() const { return as.nativeIdx; }
int Value::asProcessIdx() const { return as.processIdx; }

void printValue(const Value &value)
{
    printf("%s\n", valueToString(value).c_str());
}

std::string valueToString(const Value &value)
{
    switch (value.type)
    {
    case VAL_NULL:
        return "null";
    case VAL_BOOL:
        return value.as.boolean ? "true" : "false";
    case VAL_INT:
        return std::to_string(value.as.integer);
    case VAL_DOUBLE:
        return std::to_string(value.as.number);
    case VAL_STRING:
        return value.as.string;
    case VAL_FUNCTION:
        return "<function>";
    case VAL_NATIVE:
        return "<native>";
    case VAL_PROCESS:
        return "<process>";
    }
    return "<?>";
}
