
#pragma once

#include "value.h"
#include <string>

class SymbolTable {
public:
    virtual ~SymbolTable() = default;
    
    virtual void define(const char* name, Value value) = 0;
    virtual Value get(const char* name) = 0;
    virtual void set(const char* name, Value value) = 0;
    virtual bool contains(const char* name) const = 0;
    virtual void clear() = 0;
    

    virtual void dump() const = 0;
};