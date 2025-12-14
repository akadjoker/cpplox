#pragma once
#include <unordered_map>
#include "symboltable.h"
#include "value.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
 
// ============================================================================
// SYMBOL TABLE usando Table
// ============================================================================

class SymbolTableLua : public SymbolTable
{
public:
    void define(const char *name, Value value) override
    {
        globals_[name] = value;
    }

    Value get(const char *name) override
    {
        auto it = globals_.find(name);
        if (it == globals_.end())
        {
            return Value::makeNull();
        }
        return it->second;
    }

    void set(const char *name, Value value) override
    {
        globals_[name] = value; // Assume que já existe
    }

    bool contains(const char *name) const override
    {
        return globals_.count(name) > 0;
    }

    void clear() override
    {
        globals_.clear();
    }

    void dump() const override
    {
        for (const auto &pair : globals_)
        {
            printf("%s = ", pair.first);
            printValue(pair.second);
            printf("\n");
        }
    }

private:
    std::unordered_map<const char *, Value> globals_;
};