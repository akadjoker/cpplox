#pragma once
#include <cstdint>
#include <string>
#include "String.h"
#include <cstdint>

enum ValueType
{
    VAL_NULL,
    VAL_BOOL,
    VAL_INT,
    VAL_DOUBLE,
    VAL_STRING,
    VAL_FUNCTION
};

struct Value
{
    ValueType type;

    union
    {
        bool boolean;
        int integer;
        double number;
        const char *stringPtr;
        int functionIdx;
    } as;

    // Constructors
    Value();
    Value(const Value &other) = default;
    Value(Value &&other) noexcept = default;
    Value &operator=(const Value &other) = default;
    Value &operator=(Value &&other) noexcept = default;
    ~Value() = default;

    // Factory methods
    static Value makeNull();
    static Value makeBool(bool b);
    static Value makeTrue() { return makeBool(true); }
    static Value makeFalse() { return makeBool(false); }
    static Value makeInt(int i);
    static Value makeDouble(double d);
    static Value makeFloat(float f);
    static Value makeString(const char *str);
    static Value makeString(const std::string &str);
    static Value makeFunction(int idx);

    // Type checks
    bool isNull() const { return type == VAL_NULL; }
    bool isBool() const { return type == VAL_BOOL; }
    bool isInt() const { return type == VAL_INT; }
    bool isDouble() const { return type == VAL_DOUBLE; }
    bool isString() const { return type == VAL_STRING; }
    bool isFunction() const { return type == VAL_FUNCTION; }

    // Conversions

    bool asBool() const;
    int asInt() const;
    double asDouble() const;
    float asFloat() const;
    const char *asString() const;
    int asFunctionIdx() const;
};

void printValue(const Value &value);
std::string valueToString(const Value &value);
