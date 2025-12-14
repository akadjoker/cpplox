#pragma once
#include "chunk.h"

class Debug
{
public:
    static void disassembleChunk(const Chunk &chunk, const char *name);
    static int disassembleInstruction(const Chunk &chunk, int offset);

private:
    static int simpleInstruction(const char *name, int offset);
    static int constantInstruction(const char *name, const Chunk &chunk, int offset);
    static int byteInstruction(const char *name, const Chunk &chunk, int offset);
    static int jumpInstruction(const char *name, int sign, const Chunk &chunk, int offset);
};
