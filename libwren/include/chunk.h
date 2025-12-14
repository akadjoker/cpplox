#pragma once
#include "value.h"
#include "opcode.h"
#include <vector>
#include <string>

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;
    Chunk();

    const char* getStringPtr(size_t index) const ;
    
    void write(uint8_t byte, int line);
    int addConstant(Value value);

    size_t count() const { return code.size(); }
};

struct Function
{
    int arity;
    Chunk chunk;
    std::string name;
    bool hasReturn;
     bool isProcess; 

    Function(const std::string &n = "<script>", int a = 0);
};
