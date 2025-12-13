#include "chunk.h"
#include "vm.h"
#include "debug.h"
#include <cstdio>

void TestCalculator()
{
    printf("=== Calculator Example ===\n");
    printf("Expression: (10 + 5) * 2 - 3\n");
    printf("Expected: 27\n\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

    // (10 + 5) * 2 - 3
    int idx = chunk.addConstant(Value::makeInt(10));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);

    idx = chunk.addConstant(Value::makeInt(5));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);

    chunk.write(OP_ADD, 1);

    idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);

    chunk.write(OP_MULTIPLY, 1);

    idx = chunk.addConstant(Value::makeInt(3));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);

    chunk.write(OP_SUBTRACT, 1);

    // Print resultado
    chunk.write(OP_PRINT, 1);

    // Implicit return nil (1 byte em vez de 2!)
    chunk.write(OP_NIL, 1);
    chunk.write(OP_RETURN, 1);

    // Debug
    printf("=== Bytecode ===\n");
    Debug::disassembleChunk(chunk, "main");
    printf("\n");

    // Execute
    VM vm;
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);
}

int test_string()
{
    printf("=== String Test (SSO) ===\n\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

    // Small string (SSO inline)
    int idx1 = chunk.addConstant(Value::makeString("Hello"));
    printf("String 1: \"%s\" - size: %zu, heap: %d\n",
           chunk.constants[idx1].asString()->c_str(),
           chunk.constants[idx1].asString()->size(),
           chunk.constants[idx1].asString()->isHeap());

    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx1, 1);

    // Small string (SSO inline)
    int idx2 = chunk.addConstant(Value::makeString(" World!"));
    printf("String 2: \"%s\" - size: %zu, heap: %d\n",
           chunk.constants[idx2].asString()->c_str(),
           chunk.constants[idx2].asString()->size(),
           chunk.constants[idx2].asString()->isHeap());

    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx2, 1);

    // Concatenate
    chunk.write(OP_ADD, 1);
    chunk.write(OP_PRINT, 1);

    // Large string (heap allocated)
    int idx3 = chunk.addConstant(Value::makeString(
        "This is a very long string that exceeds SSO capacity!!!"));
    printf("String 3: \"%s...\" - size: %zu, heap: %d\n\n",
           "This is a very long string...",
           chunk.constants[idx3].asString()->size(),
           chunk.constants[idx3].asString()->isHeap());

    chunk.write(OP_CONSTANT, 2);
    chunk.write(idx3, 2);
    chunk.write(OP_PRINT, 2);

    chunk.write(OP_NIL, 3);
    chunk.write(OP_RETURN, 3);

    // Bytecode
    printf("=== Bytecode ===\n");
    Debug::disassembleChunk(chunk, "main");
    printf("\n");

    // Execute
    VM vm;
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);

    return 0;
}

int test_natives()
{

    printf("=== Manual Native Test ===\n\n");

    VM vm;

    // Registra função custom: dobro(x) = x * 2
    vm.registerNative("dobro", 1, [](VM *vm, int argc, Value *args)
                      {
        if (args[0].isInt()) {
            return Value::makeInt(args[0].asInt() * 2);
        }
        return Value::makeNull(); });

    // Registra função custom: soma3(a, b, c) = a + b + c
    vm.registerNative("soma3", 3, [](VM *vm, int argc, Value *args)
                      {
        int result = args[0].asInt() + args[1].asInt() + args[2].asInt();
        return Value::makeInt(result); });

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

    int line = 1;

    // Testa: dobro(21)
    printf("dobro(21) = ?\n");
    int valIdx = chunk.addConstant(Value::makeInt(21));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    int nameIdx = chunk.addConstant(Value::makeString("dobro"));
    chunk.write(OP_CALL_NATIVE, line);
    chunk.write(nameIdx, line);
    chunk.write(1, line); // 1 arg
    chunk.write(OP_PRINT, line);

    // Testa: soma3(10, 20, 30)
    printf("soma3(10, 20, 30) = ?\n");
    valIdx = chunk.addConstant(Value::makeInt(10));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    valIdx = chunk.addConstant(Value::makeInt(20));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    valIdx = chunk.addConstant(Value::makeInt(30));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    nameIdx = chunk.addConstant(Value::makeString("soma3"));
    chunk.write(OP_CALL_NATIVE, line);
    chunk.write(nameIdx, line);
    chunk.write(3, line); // 3 args
    chunk.write(OP_PRINT, line);

    // Testa: abs(-42)
    printf("abs(-42) = ?\n");
    valIdx = chunk.addConstant(Value::makeInt(-42));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    nameIdx = chunk.addConstant(Value::makeString("abs"));
    chunk.write(OP_CALL_NATIVE, line);
    chunk.write(nameIdx, line);
    chunk.write(1, line);
    chunk.write(OP_PRINT, line);

    // Testa: sqrt(144)
    printf("sqrt(144) = ?\n");
    valIdx = chunk.addConstant(Value::makeInt(144));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    nameIdx = chunk.addConstant(Value::makeString("sqrt"));
    chunk.write(OP_CALL_NATIVE, line);
    chunk.write(nameIdx, line);
    chunk.write(1, line);
    chunk.write(OP_PRINT, line);

    chunk.write(OP_NIL, line);
    chunk.write(OP_RETURN, line);

    printf("\n=== Results ===\n");
    vm.interpret(&mainFunc);

    return 0;
}

int main()
{
    // TestCalculator();
    // test_string();
    test_natives();
    return 0;
}