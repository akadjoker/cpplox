
#pragma once

#include "symboltable.h"
#include <unordered_map>
#include <cstring>

#define MAX_GLOBALS 512

class SymbolTableArray : public SymbolTable
{
public:
    SymbolTableArray() : count_(0) {}

    void define(const char *name, Value value) override
    {
        auto it = nameToIndex_.find(name);
        if (it != nameToIndex_.end())
        {
            globals_[it->second] = value;
            return;
        }

        if (count_ >= MAX_GLOBALS)
        {
            fprintf(stderr, "Error: Too many global variables (max %d)\n", MAX_GLOBALS);
            return;
        }

        uint16_t index = count_++;
        nameToIndex_[name] = index;
        indexToName_[index] = name;
        globals_[index] = value;
    }

    Value get(const char *name) override
    {
        auto it = nameToIndex_.find(name);
        if (it == nameToIndex_.end())
        {
            return Value::makeNull();
        }
        return globals_[it->second]; 
    }

    void set(const char *name, Value value) override
    {
        auto it = nameToIndex_.find(name);
        if (it != nameToIndex_.end())
        {
            globals_[it->second] = value;  
        }
    }

    bool contains(const char *name) const override
    {
        return nameToIndex_.count(name) > 0;
    }

    void clear() override
    {
        nameToIndex_.clear();
        indexToName_.clear();
        count_ = 0;
    }

    void dump() const override
    {
        for (uint16_t i = 0; i < count_; i++)
        {
            printf("[%d] %s = ", i, indexToName_.at(i));
            printValue(globals_[i]);
            printf("\n");
        }
    }

    // BONUS: Método para obter índice (para compiler otimizar)
    int getIndex(const char *name) const
    {
        auto it = nameToIndex_.find(name);
        if (it == nameToIndex_.end())
        {
            return -1;
        }
        return it->second;
    }

    Value getByIndex(uint16_t index) const
    {
        if (index >= count_)
        {
            return Value::makeNull();
        }
        return globals_[index];
    }

    void setByIndex(uint16_t index, Value value)
    {
        if (index < count_)
        {
            globals_[index] = value;
        }
    }

private:
    Value globals_[MAX_GLOBALS];
    std::unordered_map<const char *, uint16_t> nameToIndex_; 
    std::unordered_map<uint16_t, const char *> indexToName_;  
    uint16_t count_;
};