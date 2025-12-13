#include "chunk.h"

void Chunk::write(uint8_t byte, int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

int Chunk::addConstant(Value value)
{
    constants.push_back(value);
    return static_cast<int>(constants.size() - 1);
}

Function::Function(const std::string &n, int a)
    : arity(a), name(n) {}
