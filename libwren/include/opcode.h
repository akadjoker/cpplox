#pragma once
#include <cstdint>

#pragma once
#include <cstdint>

enum Opcode : uint8_t
{
    // Literals
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    // Stack
    OP_POP,
    OP_NOT,

    // Arithmetic
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_MODULO,

    // Comparisons
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,

    // Variables
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_DEFINE_GLOBAL,
    

    // Control flow
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,

    // Functions
    OP_CALL,
    OP_CALL_NATIVE,
    OP_RETURN,
    OP_RETURN_NIL,

    // I/O
    OP_PRINT,
};
