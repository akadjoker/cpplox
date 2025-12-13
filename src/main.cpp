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

int test_global_variables()
{
    printf("=== Global Variables Test ===\n\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

    int line = 1;

    // var x = 10;
    printf("1. var x = 10\n");
    int valIdx = chunk.addConstant(Value::makeInt(10));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    int nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_DEFINE_GLOBAL, line);
    chunk.write(nameIdx, line);

    // var y = 20;
    printf("2. var y = 20\n");
    valIdx = chunk.addConstant(Value::makeInt(20));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    nameIdx = chunk.addConstant(Value::makeString("y"));
    chunk.write(OP_DEFINE_GLOBAL, line);
    chunk.write(nameIdx, line);

    // print(x);
    printf("3. print(x)\n");
    nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_PRINT, line);

    // print(y);
    printf("4. print(y)\n");
    nameIdx = chunk.addConstant(Value::makeString("y"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_PRINT, line);

    // x = 30;
    printf("5. x = 30\n");
    valIdx = chunk.addConstant(Value::makeInt(30));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);

    nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_SET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_POP, line); // descarta valor da expressão

    // print(x);
    printf("6. print(x) - deve ser 30\n");
    nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_PRINT, line);

    // var z;  (sem inicializar)
    printf("7. var z; (nil automático)\n");
    chunk.write(OP_NIL, line);
    nameIdx = chunk.addConstant(Value::makeString("z"));
    chunk.write(OP_DEFINE_GLOBAL, line);
    chunk.write(nameIdx, line);

    // print(z);
    printf("8. print(z) - deve ser null\n");
    nameIdx = chunk.addConstant(Value::makeString("z"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_PRINT, line);

    chunk.write(OP_NIL, line);
    chunk.write(OP_RETURN, line);

    // Disassemble
    printf("\n=== Bytecode ===\n");
    Debug::disassembleChunk(chunk, "main");
    printf("\n");

    // Execute
    VM vm;
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);

    return 0;
}
 
void testRedefinition() {
    printf("=== Test 1: Redefinition Error ===\n");
    
    Function mainFunc("main");
    Chunk& chunk = mainFunc.chunk;
    
    // var x = 10;
    int valIdx = chunk.addConstant(Value::makeInt(10));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(valIdx, 1);
    
    int nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_DEFINE_GLOBAL, 1);
    chunk.write(nameIdx, 1);
    
    // var x = 20;  // ERRO!
    valIdx = chunk.addConstant(Value::makeInt(20));
    chunk.write(OP_CONSTANT, 2);
    chunk.write(valIdx, 2);
    
    nameIdx = chunk.addConstant(Value::makeString("x"));
    chunk.write(OP_DEFINE_GLOBAL, 2);
    chunk.write(nameIdx, 2);
    
    chunk.write(OP_NIL, 3);
    chunk.write(OP_RETURN, 3);
    
    VM vm;
    vm.interpret(&mainFunc);
    printf("\n");
}

void testUndefined() {
    printf("=== Test 2: Undefined Variable Error ===\n");
    
    Function mainFunc("main");
    Chunk& chunk = mainFunc.chunk;
    
    // print(nao_existe);  // ERRO!
    int nameIdx = chunk.addConstant(Value::makeString("nao_existe"));
    chunk.write(OP_GET_GLOBAL, 1);
    chunk.write(nameIdx, 1);
    chunk.write(OP_PRINT, 1);
    
    chunk.write(OP_NIL, 2);
    chunk.write(OP_RETURN, 2);
    
    VM vm;
    vm.interpret(&mainFunc);
    printf("\n");
}

void testAssignUndefined() {
    printf("=== Test 3: Assign to Undefined Error ===\n");
    
    Function mainFunc("main");
    Chunk& chunk = mainFunc.chunk;
    
    // y = 100;  // ERRO! y não foi definida
    int valIdx = chunk.addConstant(Value::makeInt(100));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(valIdx, 1);
    
    int nameIdx = chunk.addConstant(Value::makeString("y"));
    chunk.write(OP_SET_GLOBAL, 1);
    chunk.write(nameIdx, 1);
    
    chunk.write(OP_NIL, 2);
    chunk.write(OP_RETURN, 2);
    
    VM vm;
    vm.interpret(&mainFunc);
    printf("\n");
}

 
int loopWithGlobals() {
    printf("=== Loop with Globals Test ===\n\n");
    printf("Code:\n");
    printf("var counter = 0;\n");
    printf("while (counter < 5) {\n");
    printf("    print(counter);\n");
    printf("    counter = counter + 1;\n");
    printf("}\n\n");
    
    Function mainFunc("main");
    Chunk& chunk = mainFunc.chunk;
    
    int line = 1;
    
    // var counter = 0;
    int valIdx = chunk.addConstant(Value::makeInt(0));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);
    
    int nameIdx = chunk.addConstant(Value::makeString("counter"));
    chunk.write(OP_DEFINE_GLOBAL, line);
    chunk.write(nameIdx, line);
    
    // LOOP START
    int loopStart = chunk.count();
    
    // while (counter < 5)
    nameIdx = chunk.addConstant(Value::makeString("counter"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    
    valIdx = chunk.addConstant(Value::makeInt(5));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);
    
    chunk.write(OP_LESS, line);
    
    // if false, jump to end
    chunk.write(OP_JUMP_IF_FALSE, line);
    int exitJump = chunk.count();
    chunk.write(0, line);  // placeholder
    chunk.write(0, line);
    chunk.write(OP_POP, line);  // pop condition
    
    // print(counter);
    nameIdx = chunk.addConstant(Value::makeString("counter"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_PRINT, line);
    
    // counter = counter + 1;
    nameIdx = chunk.addConstant(Value::makeString("counter"));
    chunk.write(OP_GET_GLOBAL, line);
    chunk.write(nameIdx, line);
    
    valIdx = chunk.addConstant(Value::makeInt(1));
    chunk.write(OP_CONSTANT, line);
    chunk.write(valIdx, line);
    
    chunk.write(OP_ADD, line);
    
    nameIdx = chunk.addConstant(Value::makeString("counter"));
    chunk.write(OP_SET_GLOBAL, line);
    chunk.write(nameIdx, line);
    chunk.write(OP_POP, line);  // descarta resultado
    
    // Loop back
    chunk.write(OP_LOOP, line);
    int loopOffset = chunk.count() - loopStart + 2;
    chunk.write((loopOffset >> 8) & 0xff, line);
    chunk.write(loopOffset & 0xff, line);
    
    // EXIT (patch jump)
    int exitOffset = chunk.count() - exitJump - 2;
    chunk.code[exitJump] = (exitOffset >> 8) & 0xff;
    chunk.code[exitJump + 1] = exitOffset & 0xff;
    
    chunk.write(OP_POP, line);  // pop condition
    
    chunk.write(OP_NIL, line);
    chunk.write(OP_RETURN, line);
    
    // Execute
    VM vm;
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);
    
    return 0;
}

    int main()
    {
        // TestCalculator();
        // test_string();
    //test_natives();

    //test_global_variables();

      //   testRedefinition();
   //  testUndefined();
    // testAssignUndefined();

    loopWithGlobals();


        return 0;
    }