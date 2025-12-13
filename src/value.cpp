#include "value.h"
#include <cstdio>
#include "value.h"
#include <cstdio>

Value::Value() : type(VAL_NULL)
{
    as.integer = 0;
}

Value::Value(const Value &other) : type(other.type)
{
    if (type == VAL_STRING)
    {
        as.string = new String(*other.as.string);
    }
    else
    {
        as = other.as;
    }
}

Value::Value(Value &&other) noexcept : type(other.type)
{
    as = other.as;
    if (type == VAL_STRING)
    {
        other.as.string = nullptr;
    }
}

Value &Value::operator=(const Value &other)
{
    if (this != &other)
    {
        if (type == VAL_STRING)
        {
            delete as.string;
        }

        type = other.type;
        if (type == VAL_STRING)
        {
            as.string = new String(*other.as.string);
        }
        else
        {
            as = other.as;
        }
    }
    return *this;
}

Value &Value::operator=(Value &&other) noexcept
{
    if (this != &other)
    {
        if (type == VAL_STRING)
        {
            delete as.string;
        }

        type = other.type;
        as = other.as;

        if (type == VAL_STRING)
        {
            other.as.string = nullptr;
        }
    }
    return *this;
}

Value::~Value()
{
    if (type == VAL_STRING)
    {
        delete as.string;
    }
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
    v.as.string = new String(str);
    return v;
}

Value Value::makeString(const String &str)
{
    Value v;
    v.type = VAL_STRING;
    v.as.string = new String(str);
    return v;
}

Value Value::makeFunction(int idx)
{
    Value v;
    v.type = VAL_FUNCTION;
    v.as.functionIdx = idx;
    return v;
}

bool Value::asBool() const { return as.boolean; }
int Value::asInt() const { return as.integer; }
double Value::asDouble() const { return as.number; }
float Value::asFloat() const { return (float)as.number; }
String *Value::asString() const { return as.string; }
int Value::asFunctionIdx() const { return as.functionIdx; }

void printValue(const Value &value)
{
    printf("%s", valueToString(value).c_str());
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
        return value.as.string->c_str();
    case VAL_FUNCTION:
        return "<fn>";
    }
    return "<?>";
}
