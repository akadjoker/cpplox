#include "chunk.h"

void Chunk::write(uint8_t byte, int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

Chunk::Chunk()
{
    code.reserve(256);  
    constants.reserve(64);
    lines.reserve(256);
}

const char *Chunk::getStringPtr(size_t index) const
{
    const Value &v = constants[index];
    if (v.type == VAL_STRING)
    {
        return v.as.string;
    }
    return nullptr;
}

int Chunk::addConstant(Value value)
{
    constants.push_back(value);
    return static_cast<int>(constants.size() - 1);
}

Function::Function(const std::string &n, int a)
    : arity(a), name(n), hasReturn(false) {}
