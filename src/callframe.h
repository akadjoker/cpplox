#pragma once
#include "chunk.h"

struct CallFrame
{
    Function *function;
    uint8_t *ip;
    Value *slots;

    CallFrame();
};
