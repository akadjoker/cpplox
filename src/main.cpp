#include "chunk.h"
#include "vm.h"
#include "debug.h"
#include <cstdio>
#include <ctime>
#include <chrono>


using namespace std::chrono;

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

void testRedefinition()
{
    printf("=== Test 1: Redefinition Error ===\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

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

void testUndefined()
{
    printf("=== Test 2: Undefined Variable Error ===\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

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

void testAssignUndefined()
{
    printf("=== Test 3: Assign to Undefined Error ===\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

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

int loopWithGlobals()
{
    printf("=== Loop with Globals Test ===\n\n");
    printf("Code:\n");
    printf("var counter = 0;\n");
    printf("while (counter < 5) {\n");
    printf("    print(counter);\n");
    printf("    counter = counter + 1;\n");
    printf("}\n\n");

    Function mainFunc("main");
    Chunk &chunk = mainFunc.chunk;

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
    chunk.write(0, line); // placeholder
    chunk.write(0, line);
    chunk.write(OP_POP, line); // pop condition

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
    chunk.write(OP_POP, line); // descarta resultado

    // Loop back
    chunk.write(OP_LOOP, line);
    int loopOffset = chunk.count() - loopStart + 2;
    chunk.write((loopOffset >> 8) & 0xff, line);
    chunk.write(loopOffset & 0xff, line);

    // EXIT (patch jump)
    int exitOffset = chunk.count() - exitJump - 2;
    chunk.code[exitJump] = (exitOffset >> 8) & 0xff;
    chunk.code[exitJump + 1] = exitOffset & 0xff;

    chunk.write(OP_POP, line); // pop condition

    chunk.write(OP_NIL, line);
    chunk.write(OP_RETURN, line);

    // Execute
    VM vm;
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);

    return 0;
}

int teste_functions()
{
    printf("=== Function Call Test ===\n\n");

    VM vm;

    // ===== FUNÇÃO: add(a, b) =====
    Function *addFunc = new Function("add", 2); // arity = 2
    Chunk &addChunk = addFunc->chunk;

    // return a + b;
    addChunk.write(OP_GET_LOCAL, 1);
    addChunk.write(0, 1); // slot 0 = a

    addChunk.write(OP_GET_LOCAL, 1);
    addChunk.write(1, 1); // slot 1 = b

    addChunk.write(OP_ADD, 1);
    addChunk.write(OP_RETURN, 1);

    // Registra função
    vm.registerFunction("add", addFunc);

    printf("Registered function: add(a, b)\n");

    // ===== FUNÇÃO MAIN =====
    Function mainFunc("main");
    Chunk &mainChunk = mainFunc.chunk;

    // print(add(10, 5));
    printf("Code: print(add(10, 5))\n\n");

    int valIdx = mainChunk.addConstant(Value::makeInt(10));
    mainChunk.write(OP_CONSTANT, 1);
    mainChunk.write(valIdx, 1);

    valIdx = mainChunk.addConstant(Value::makeInt(5));
    mainChunk.write(OP_CONSTANT, 1);
    mainChunk.write(valIdx, 1);

    int nameIdx = mainChunk.addConstant(Value::makeString("add"));
    mainChunk.write(OP_CALL, 1);
    mainChunk.write(nameIdx, 1);
    mainChunk.write(2, 1); // 2 args

    mainChunk.write(OP_PRINT, 1);

    // print(add(100, 200));
    printf("Code: print(add(100, 200))\n\n");

    valIdx = mainChunk.addConstant(Value::makeInt(100));
    mainChunk.write(OP_CONSTANT, 2);
    mainChunk.write(valIdx, 2);

    valIdx = mainChunk.addConstant(Value::makeInt(200));
    mainChunk.write(OP_CONSTANT, 2);
    mainChunk.write(valIdx, 2);

    nameIdx = mainChunk.addConstant(Value::makeString("add"));
    mainChunk.write(OP_CALL, 2);
    mainChunk.write(nameIdx, 2);
    mainChunk.write(2, 2);

    mainChunk.write(OP_PRINT, 2);

    mainChunk.write(OP_NIL, 3);
    mainChunk.write(OP_RETURN, 3);

    // Execute
    printf("=== Execution ===\n");
    vm.interpret(&mainFunc);

    return 0;
}

int teste_fib()
{
    printf("=== Fibonacci Recursivo ===\n\n");
    printf("fn fib(n) {\n");
    printf("    if (n < 2) return n;\n");
    printf("    return fib(n-1) + fib(n-2);\n");
    printf("}\n\n");

    VM vm;

    // ===== FUNÇÃO: fib(n) =====
    Function *fibFunc = new Function("fib", 1); // arity = 1
    Chunk &fibChunk = fibFunc->chunk;

    int line = 1;

    // if (n < 2)
    fibChunk.write(OP_GET_LOCAL, line);
    fibChunk.write(0, line); // slot 0 = n

    int constIdx = fibChunk.addConstant(Value::makeInt(2));
    fibChunk.write(OP_CONSTANT, line);
    fibChunk.write(constIdx, line);

    fibChunk.write(OP_LESS, line); // n < 2

    fibChunk.write(OP_JUMP_IF_FALSE, line);
    int elseJump = fibChunk.count();
    fibChunk.write(0, line); // placeholder
    fibChunk.write(0, line);
    fibChunk.write(OP_POP, line); // pop condition

    // THEN: return n;
    fibChunk.write(OP_GET_LOCAL, line);
    fibChunk.write(0, line); // return n
    fibChunk.write(OP_RETURN, line);

    // ELSE: patch jump
    int elseOffset = fibChunk.count() - elseJump - 2;
    fibChunk.code[elseJump] = (elseOffset >> 8) & 0xff;
    fibChunk.code[elseJump + 1] = elseOffset & 0xff;
    fibChunk.write(OP_POP, line); // pop condition

    // return fib(n-1) + fib(n-2);

    // fib(n-1)
    fibChunk.write(OP_GET_LOCAL, line);
    fibChunk.write(0, line); // n

    constIdx = fibChunk.addConstant(Value::makeInt(1));
    fibChunk.write(OP_CONSTANT, line);
    fibChunk.write(constIdx, line);

    fibChunk.write(OP_SUBTRACT, line); // n - 1

    int nameIdx = fibChunk.addConstant(Value::makeString("fib"));
    fibChunk.write(OP_CALL, line);
    fibChunk.write(nameIdx, line);
    fibChunk.write(1, line); // 1 arg

    // fib(n-2)
    fibChunk.write(OP_GET_LOCAL, line);
    fibChunk.write(0, line); // n

    constIdx = fibChunk.addConstant(Value::makeInt(2));
    fibChunk.write(OP_CONSTANT, line);
    fibChunk.write(constIdx, line);

    fibChunk.write(OP_SUBTRACT, line); // n - 2

    nameIdx = fibChunk.addConstant(Value::makeString("fib"));
    fibChunk.write(OP_CALL, line);
    fibChunk.write(nameIdx, line);
    fibChunk.write(1, line); // 1 arg

    // fib(n-1) + fib(n-2)
    fibChunk.write(OP_ADD, line);
    fibChunk.write(OP_RETURN, line);

    // Registra função
    vm.registerFunction("fib", fibFunc);

    // ===== MAIN =====
    Function mainFunc("main");
    Chunk &mainChunk = mainFunc.chunk;

    // Testa vários valores
    for (int n = 0; n <= 20; n++)
    {
        printf("Calculando fib(%d)...\n", n);

        constIdx = mainChunk.addConstant(Value::makeInt(n));
        mainChunk.write(OP_CONSTANT, 1);
        mainChunk.write(constIdx, 1);

        nameIdx = mainChunk.addConstant(Value::makeString("fib"));
        mainChunk.write(OP_CALL, 1);
        mainChunk.write(nameIdx, 1);
        mainChunk.write(1, 1);

        mainChunk.write(OP_PRINT, 1);
    }

    mainChunk.write(OP_NIL, 1);
    mainChunk.write(OP_RETURN, 1);

    // Debug
    printf("\n=== Bytecode da função fib ===\n");
    Debug::disassembleChunk(fibChunk, "fib");

    // Execute
    printf("\n=== Execution ===\n");
    vm.interpret(&mainFunc);

    return 0;
}

int test_api()
{
    VM vm;

    // Push values
    vm.PushInt(10);
    vm.PushString("hello");
    vm.PushDouble(3.14);

    // Lua-style indexing
    printf("Base: %d\n", vm.ToInt(0));      // 10
    printf("Top: %.2f\n", vm.ToDouble(-1)); // 3.14

    // Type check
    if (vm.IsString(1))
    {
        printf("String: %s\n", vm.ToString(1)); // hello
    }

    // Stack info
    printf("Size: %d\n", vm.GetTop()); // 3

    // Debug
    vm.DumpStack();

    return 0;
}

int test_pcall()
{
    printf("=== Testing GetGlobal() and Call() ===\n\n");

    VM vm;

    // ===== 1. CRIAR FUNÇÃO: add(a, b) =====
    Function *addFunc = new Function("add", 2);
    Chunk &chunk = addFunc->chunk;

    // return a + b;
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1); // a

    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(1, 1); // b

    chunk.write(OP_ADD, 1);
    chunk.write(OP_RETURN, 1);

    // Registra função
    uint16_t addIdx = vm.registerFunction("add", addFunc);
    printf("Registered 'add' with index: %d\n\n", addIdx);

    // ===== 2. CRIAR GLOBAL COM FUNÇÃO =====
    vm.Push(Value::makeFunction(addIdx));
    vm.SetGlobal("add");
    printf("Set global 'add'\n\n");

    // ===== 3. TESTAR GetGlobal() =====
    printf("Test 1: GetGlobal\n");
    vm.GetGlobal("add");

    if (vm.IsFunction(-1))
    {
        printf("✅ GetGlobal('add') returned function\n");
        vm.Pop();
    }
    else
    {
        printf("❌ GetGlobal('add') failed\n");
        return 1;
    }

    // ===== 4. TESTAR Call() =====
    printf("\nTest 2: Call add(10, 20)\n");

    vm.GetGlobal("add"); // push função
    vm.PushInt(10);      // arg1
    vm.PushInt(20);      // arg2

    printf("Stack before Call: size=%d\n", vm.GetTop());
    vm.DumpStack();

    vm.Call(2, 1); // 2 args, 1 result

    printf("\nStack after Call: size=%d\n", vm.GetTop());
    vm.DumpStack();

    if (vm.IsInt(-1))
    {
        int result = vm.ToInt(-1);
        printf("\n✅ Result: %d\n", result);

        if (result == 30)
        {
            printf("✅ CORRECT! 10 + 20 = 30\n");
        }
        else
        {
            printf("❌ WRONG! Expected 30, got %d\n", result);
        }
        vm.Pop();
    }
    else
    {
        printf("❌ Call failed\n");
        return 1;
    }

    // ===== 5. TESTAR Call SEM RESULTADO =====
    printf("\n\nTest 3: Call add(5, 3) with no result\n");

    vm.GetGlobal("add");
    vm.PushInt(5);
    vm.PushInt(3);

    vm.Call(2, 0); // 0 results - descarta

    printf("Stack size after Call(2,0): %d\n", vm.GetTop());
    if (vm.GetTop() == 0)
    {
        printf("✅ Result discarded correctly\n");
    }

    // ===== 6. TESTAR FUNÇÃO RECURSIVA =====
    printf("\n\nTest 4: Recursive function - fib(5)\n");

    Function *fibFunc = new Function("fib", 1);
    Chunk &fibChunk = fibFunc->chunk;

    // if (n < 2) return n;
    fibChunk.write(OP_GET_LOCAL, 1);
    fibChunk.write(0, 1);

    int idx = fibChunk.addConstant(Value::makeInt(2));
    fibChunk.write(OP_CONSTANT, 1);
    fibChunk.write(idx, 1);

    fibChunk.write(OP_LESS, 1);
    fibChunk.write(OP_JUMP_IF_FALSE, 1);
    int elseJump = fibChunk.count();
    fibChunk.write(0, 1);
    fibChunk.write(0, 1);
    fibChunk.write(OP_POP, 1);

    fibChunk.write(OP_GET_LOCAL, 1);
    fibChunk.write(0, 1);
    fibChunk.write(OP_RETURN, 1);

    int offset = fibChunk.count() - elseJump - 2;
    fibChunk.code[elseJump] = (offset >> 8) & 0xff;
    fibChunk.code[elseJump + 1] = offset & 0xff;
    fibChunk.write(OP_POP, 1);

    // return fib(n-1) + fib(n-2);
    fibChunk.write(OP_GET_LOCAL, 1);
    fibChunk.write(0, 1);
    idx = fibChunk.addConstant(Value::makeInt(1));
    fibChunk.write(OP_CONSTANT, 1);
    fibChunk.write(idx, 1);
    fibChunk.write(OP_SUBTRACT, 1);

    idx = fibChunk.addConstant(Value::makeString("fib"));
    fibChunk.write(OP_CALL, 1);
    fibChunk.write(idx, 1);
    fibChunk.write(1, 1);

    fibChunk.write(OP_GET_LOCAL, 1);
    fibChunk.write(0, 1);
    idx = fibChunk.addConstant(Value::makeInt(2));
    fibChunk.write(OP_CONSTANT, 1);
    fibChunk.write(idx, 1);
    fibChunk.write(OP_SUBTRACT, 1);

    idx = fibChunk.addConstant(Value::makeString("fib"));
    fibChunk.write(OP_CALL, 1);
    fibChunk.write(idx, 1);
    fibChunk.write(1, 1);

    fibChunk.write(OP_ADD, 1);
    fibChunk.write(OP_RETURN, 1);

    uint16_t fibIdx = vm.registerFunction("fib", fibFunc);
    vm.Push(Value::makeFunction(fibIdx));
    vm.SetGlobal("fib");

    // Call fib(5)
    vm.GetGlobal("fib");
    vm.PushInt(5);
    vm.Call(1, 1);

    int fibResult = vm.ToInt(-1);
    printf("fib(5) = %d\n", fibResult);

    if (fibResult == 5)
    {
        printf("✅ CORRECT! fib(5) = 5\n");
    }
    else
    {
        printf("❌ WRONG! Expected 5, got %d\n", fibResult);
    }

    printf("\n\n=== ALL TESTS COMPLETE ===\n");

    return 0;
}

// int main()
// {
//     TestCalculator();
//     test_string();
//     test_natives();

//     test_global_variables();

//     testRedefinition();
//     testUndefined();
//     testAssignUndefined();

//     loopWithGlobals();
//     teste_functions();

//     teste_fib();

//     test_api();

//     test_pcall();

//     return 0;
// }



// ============================================
// TEST 1: Stack Overflow
// ============================================

// void testStackOverflow() {
//     printf("\n=== TEST 1: Stack Overflow ===\n");
    
//     VM vm;
    
//     // Empilha 1000 valores
//     printf("Pushing 1000 values...\n");
//     for (int i = 0; i < 1000; i++) {
//         vm.PushInt(i);
//     }
    
//     printf("Stack size: %d\n", vm.GetTop());
    
//     if (vm.GetTop() == 256) {
//         printf("⚠️  Stack saturated at 256 (expected)\n");
//     } else if (vm.GetTop() == 1000) {
//         printf("❌ BUFFER OVERFLOW! Stack should be 256 max!\n");
//     }
// }

// // ============================================
// // TEST 2: Stack Underflow
// // ============================================

// void testStackUnderflow() {
//     printf("\n=== TEST 2: Stack Underflow ===\n");
    
//     VM vm;
    
//     printf("Popping from empty stack...\n");
//     Value v = vm.Pop();  // deve dar erro!
    
//     if (v.isNull()) {
//         printf("✅ Returned null on underflow\n");
//     } else {
//         printf("❌ Should return null or error!\n");
//     }
    
//     // Pop 100x
//     printf("Popping 100 times from empty...\n");
//     for (int i = 0; i < 100; i++) {
//         vm.Pop();
//     }
    
//     printf("✅ Survived 100 underflows\n");
// }

// // ============================================
// // TEST 3: Recursão Extrema
// // ============================================

// void testDeepRecursion() {
//     printf("\n=== TEST 3: Deep Recursion (fib) ===\n");
    
//     VM vm;
    
//     // Função fib (como antes)
//     Function* fibFunc = new Function("fib", 1);
//     Chunk& chunk = fibFunc->chunk;
    
//     // if (n < 2) return n;
//     chunk.write(OP_GET_LOCAL, 1);
//     chunk.write(0, 1);
//     int idx = chunk.addConstant(Value::makeInt(2));
//     chunk.write(OP_CONSTANT, 1);
//     chunk.write(idx, 1);
//     chunk.write(OP_LESS, 1);
//     chunk.write(OP_JUMP_IF_FALSE, 1);
//     int elseJump = chunk.count();
//     chunk.write(0, 1);
//     chunk.write(0, 1);
//     chunk.write(OP_POP, 1);
//     chunk.write(OP_GET_LOCAL, 1);
//     chunk.write(0, 1);
//     chunk.write(OP_RETURN, 1);
    
//     int offset = chunk.count() - elseJump - 2;
//     chunk.code[elseJump] = (offset >> 8) & 0xff;
//     chunk.code[elseJump + 1] = offset & 0xff;
//     chunk.write(OP_POP, 1);
    
//     // return fib(n-1) + fib(n-2);
//     chunk.write(OP_GET_LOCAL, 1);
//     chunk.write(0, 1);
//     idx = chunk.addConstant(Value::makeInt(1));
//     chunk.write(OP_CONSTANT, 1);
//     chunk.write(idx, 1);
//     chunk.write(OP_SUBTRACT, 1);
//     idx = chunk.addConstant(Value::makeString("fib"));
//     chunk.write(OP_CALL, 1);
//     chunk.write(idx, 1);
//     chunk.write(1, 1);
    
//     chunk.write(OP_GET_LOCAL, 1);
//     chunk.write(0, 1);
//     idx = chunk.addConstant(Value::makeInt(2));
//     chunk.write(OP_CONSTANT, 1);
//     chunk.write(idx, 1);
//     chunk.write(OP_SUBTRACT, 1);
//     idx = chunk.addConstant(Value::makeString("fib"));
//     chunk.write(OP_CALL, 1);
//     chunk.write(idx, 1);
//     chunk.write(1, 1);
    
//     chunk.write(OP_ADD, 1);
//     chunk.write(OP_RETURN, 1);
    
//     uint16_t fibIdx = vm.registerFunction("fib", fibFunc);
//     vm.Push(Value::makeFunction(fibIdx));
//     vm.SetGlobal("fib");
    
//     // Testa valores crescentes
//     int tests[] = {5, 10, 15, 20, 25, 30};
    
//     for (int i = 0; i < 6; i++) {
//         int n = tests[i];
//         printf("Testing fib(%d)...", n);
        
//         clock_t start = clock();
        
//         vm.GetGlobal("fib");
//         vm.PushInt(n);
//         vm.Call(1, 1);
        
//         clock_t end = clock();
//         double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
//         int result = vm.ToInt(-1);
//         vm.Pop();
        
//         printf(" = %d (%.2f ms)\n", result, time);
        
//         if (time > 1000.0) {
//             printf("⚠️  Took > 1 second! Stopping.\n");
//             break;
//         }
//     }
// }

// // ============================================
// // TEST 4: Call Stack Overflow
// // ============================================

// void testCallStackOverflow() {
//     printf("\n=== TEST 4: Call Stack Overflow ===\n");
    
//     VM vm;
    
//     // Função recursiva infinita: boom() { boom(); }
//     Function* boomFunc = new Function("boom", 0);
//     Chunk& chunk = boomFunc->chunk;
    
//     int idx = chunk.addConstant(Value::makeString("boom"));
//     chunk.write(OP_CALL, 1);
//     chunk.write(idx, 1);
//     chunk.write(0, 1);  // 0 args
    
//     chunk.write(OP_NIL, 1);
//     chunk.write(OP_RETURN, 1);
    
//     uint16_t boomIdx = vm.registerFunction("boom", boomFunc);
//     vm.Push(Value::makeFunction(boomIdx));
//     vm.SetGlobal("boom");
    
//     printf("Calling infinite recursion boom()...\n");
    
//     vm.GetGlobal("boom");
//     vm.Call(0, 0);  // Deve dar stack overflow!
    
//     printf("✅ Survived (should have errored)\n");
// }

// // ============================================
// // TEST 5: Type Confusion
// // ============================================

// void testTypeErrors() {
//     printf("\n=== TEST 5: Type Errors ===\n");
    
//     VM vm;
    
//     // String + Int?
//     printf("Test: string + int...\n");
//     vm.PushString("hello");
//     vm.PushInt(42);
//     // Como fazer ADD sem bytecode? Precisa de função helper
    
//     // Int como função?
//     printf("Test: call int as function...\n");
//     vm.PushInt(123);
//     vm.Call(0, 0);  // Deve dar erro!
    
//     // ToInt de string?
//     printf("Test: ToInt of string...\n");
//     vm.PushString("not a number");
//     int n = vm.ToInt(-1);  // Deve dar erro!
//     printf("Got: %d\n", n);
// }

// // ============================================
// // TEST 6: Memory Stress
// // ============================================

// void testMemoryStress() {
//     printf("\n=== TEST 6: Memory Stress (Strings) ===\n");
    
//     VM vm;
    
//     printf("Creating 10000 strings...\n");
    
//     for (int i = 0; i < 10000; i++) {
//         char buf[64];
//         snprintf(buf, 64, "string_number_%d", i);
        
//         vm.PushString(buf);
//         vm.SetGlobal(buf);  // Guarda global
        
//         if (i % 1000 == 0) {
//             printf("  %d strings created...\n", i);
//         }
//     }
    
//     printf("✅ Created 10000 strings\n");
//     printf("⚠️  Memory leak? Check with valgrind!\n");
    
//     // Limpa
//     printf("Clearing stack...\n");
//     while (vm.GetTop() > 0) {
//         vm.Pop();
//     }
// }

// // ============================================
// // TEST 7: Invalid Indices
// // ============================================

// void testInvalidIndices() {
//     printf("\n=== TEST 7: Invalid Stack Indices ===\n");
    
//     VM vm;
    
//     vm.PushInt(10);
//     vm.PushInt(20);
    
//     printf("Stack size: %d\n", vm.GetTop());
    
//     // Acesso inválido
//     printf("Test: Peek(999)...\n");
//     const Value& v1 = vm.Peek(999);
//     printf("Result type: %s\n", vm.TypeName(v1.type));
    
//     printf("Test: Peek(-999)...\n");
//     const Value& v2 = vm.Peek(-999);
//     printf("Result type: %s\n", vm.TypeName(v2.type));
    
//     printf("Test: ToInt(50)...\n");
//     int n = vm.ToInt(50);
//     printf("Result: %d\n", n);
// }

// // ============================================
// // TEST 8: Global Overwrite
// // ============================================

// void testGlobalOverwrite() {
//     printf("\n=== TEST 8: Global Overwrite ===\n");
    
//     VM vm;
    
//     // Set global várias vezes
//     for (int i = 0; i < 100; i++) {
//         vm.PushInt(i);
//         vm.SetGlobal("x");
//     }
    
//     vm.GetGlobal("x");
//     int result = vm.ToInt(-1);
//     vm.Pop();
    
//     printf("Final value of x: %d\n", result);
    
//     if (result == 99) {
//         printf("✅ Correctly overwrote\n");
//     } else {
//         printf("❌ Expected 99, got %d\n", result);
//     }
    
//     printf("⚠️  99 strings leaked? Need GC!\n");
// }

// // ============================================
// // TEST 9: Nested Calls
// // ============================================

// void testNestedCalls() {
//     printf("\n=== TEST 9: Nested Calls ===\n");
    
//     VM vm;
    
//     // a() calls b() calls c()
//     Function* cFunc = new Function("c", 0);
//     cFunc->chunk.write(OP_CONSTANT, 1);
//     cFunc->chunk.write(cFunc->chunk.addConstant(Value::makeInt(42)), 1);
//     cFunc->chunk.write(OP_RETURN, 1);
    
//     Function* bFunc = new Function("b", 0);
//     int idx = bFunc->chunk.addConstant(Value::makeString("c"));
//     bFunc->chunk.write(OP_CALL, 1);
//     bFunc->chunk.write(idx, 1);
//     bFunc->chunk.write(0, 1);
//     bFunc->chunk.write(OP_RETURN, 1);
    
//     Function* aFunc = new Function("a", 0);
//     idx = aFunc->chunk.addConstant(Value::makeString("b"));
//     aFunc->chunk.write(OP_CALL, 1);
//     aFunc->chunk.write(idx, 1);
//     aFunc->chunk.write(0, 1);
//     aFunc->chunk.write(OP_RETURN, 1);
    
//     vm.registerFunction("c", cFunc);
//     vm.registerFunction("b", bFunc);
//     vm.registerFunction("a", aFunc);
    
//     vm.Push(Value::makeFunction(vm.registerFunction("a", aFunc)));
//     vm.SetGlobal("a");
//     vm.Push(Value::makeFunction(vm.registerFunction("b", bFunc)));
//     vm.SetGlobal("b");
//     vm.Push(Value::makeFunction(vm.registerFunction("c", cFunc)));
//     vm.SetGlobal("c");
    
//     printf("Calling a() -> b() -> c()...\n");
    
//     vm.GetGlobal("a");
//     vm.Call(0, 1);
    
//     int result = vm.ToInt(-1);
//     printf("Result: %d\n", result);
    
//     if (result == 42) {
//         printf("✅ Nested calls work!\n");
//     }
// }

// // ============================================
// // TEST 10: Concurrent-like Stress
// // ============================================

// void testRapidFireCalls() {
//     printf("\n=== TEST 10: Rapid Fire Calls ===\n");
    
//     VM vm;
    
//     // Simple add function
//     Function* addFunc = new Function("add", 2);
//     addFunc->chunk.write(OP_GET_LOCAL, 1);
//     addFunc->chunk.write(0, 1);
//     addFunc->chunk.write(OP_GET_LOCAL, 1);
//     addFunc->chunk.write(1, 1);
//     addFunc->chunk.write(OP_ADD, 1);
//     addFunc->chunk.write(OP_RETURN, 1);
    
//     uint16_t addIdx = vm.registerFunction("add", addFunc);
//     vm.Push(Value::makeFunction(addIdx));
//     vm.SetGlobal("add");
    
//     printf("Calling add() 10000 times...\n");
    
//     clock_t start = clock();
    
//     for (int i = 0; i < 10000; i++) {
//         vm.GetGlobal("add");
//         vm.PushInt(i);
//         vm.PushInt(i + 1);
//         vm.Call(2, 1);
//         vm.Pop();  // descarta resultado
//     }
    
//     clock_t end = clock();
//     double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
//     printf("✅ 10000 calls in %.2f ms\n", time);
//     printf("   %.2f calls/ms\n", 10000.0 / time);
// }

// ============================================
// MAIN - Roda Todos
// ============================================

// int main() {
//     printf("╔════════════════════════════════════╗\n");
//     printf("║   VM STRESS TESTS - REBENTA ISTO  ║\n");
//     printf("╚════════════════════════════════════╝\n");
    
//     testStackOverflow();
//     testStackUnderflow();
//     testDeepRecursion();
//     testCallStackOverflow();
//     testTypeErrors();
//     testMemoryStress();
//     testInvalidIndices();
//     testGlobalOverwrite();
//     testNestedCalls();
//     testRapidFireCalls();
    
//     printf("\n╔════════════════════════════════════╗\n");
//     printf("║       STRESS TESTS COMPLETE        ║\n");
//     printf("╚════════════════════════════════════╝\n");
//     printf("\nRun with valgrind to check leaks:\n");
//     printf("  valgrind --leak-check=full ./bin/stress_tests\n");
    
//     return 0;
// }



// ============================================
// TEST 1: Stack Overflow
// ============================================

void testStackOverflow() {
    printf("\n=== TEST 1: Stack Overflow ===\n");
    
    VM vm;
    
    // Empilha 1000 valores
    printf("Pushing 1000 values...\n");
    for (int i = 0; i < 1000; i++) {
        vm.PushInt(i);
    }
    
    printf("Stack size: %d\n", vm.GetTop());
    
    if (vm.GetTop() == 256) {
        printf("⚠️  Stack saturated at 256 (expected)\n");
    } else if (vm.GetTop() == 1000) {
        printf("❌ BUFFER OVERFLOW! Stack should be 256 max!\n");
    }
}

// ============================================
// TEST 2: Stack Underflow
// ============================================

void testStackUnderflow() {
    printf("\n=== TEST 2: Stack Underflow ===\n");
    
    VM vm;
    
    printf("Popping from empty stack...\n");
    Value v = vm.Pop();  // deve dar erro!
    
    if (v.isNull()) {
        printf("✅ Returned null on underflow\n");
    } else {
        printf("❌ Should return null or error!\n");
    }
    
    // Pop 100x
    printf("Popping 100 times from empty...\n");
    for (int i = 0; i < 100; i++) {
        vm.Pop();
    }
    
    printf("✅ Survived 100 underflows\n");
}

// ============================================
// TEST 3: Recursão Extrema
// ============================================

void testDeepRecursion() {
    printf("\n=== TEST 3: Deep Recursion (fib) ===\n");
    
    VM vm;
    
    // Função fib (como antes)
    Function* fibFunc = new Function("fib", 1);
    Chunk& chunk = fibFunc->chunk;
    
    // if (n < 2) return n;
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    int idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_LESS, 1);
    chunk.write(OP_JUMP_IF_FALSE, 1);
    int elseJump = chunk.count();
    chunk.write(0, 1);
    chunk.write(0, 1);
    chunk.write(OP_POP, 1);
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    chunk.write(OP_RETURN, 1);
    
    int offset = chunk.count() - elseJump - 2;
    chunk.code[elseJump] = (offset >> 8) & 0xff;
    chunk.code[elseJump + 1] = offset & 0xff;
    chunk.write(OP_POP, 1);
    
    // return fib(n-1) + fib(n-2);
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(1));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    idx = chunk.addConstant(Value::makeString("fib"));
    chunk.write(OP_CALL, 1);
    chunk.write(idx, 1);
    chunk.write(1, 1);
    
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    idx = chunk.addConstant(Value::makeString("fib"));
    chunk.write(OP_CALL, 1);
    chunk.write(idx, 1);
    chunk.write(1, 1);
    
    chunk.write(OP_ADD, 1);
    chunk.write(OP_RETURN, 1);
    
    uint16_t fibIdx = vm.registerFunction("fib", fibFunc);
    vm.Push(Value::makeFunction(fibIdx));
    vm.SetGlobal("fib");
    
    // Testa valores crescentes
    int tests[] = {5, 10, 15, 20, 25, 30};
    
    for (int i = 0; i < 6; i++) {
        int n = tests[i];
        printf("Testing fib(%d)...", n);
        
        clock_t start = clock();
        
        vm.GetGlobal("fib");
        vm.PushInt(n);
        vm.Call(1, 1);
        
        clock_t end = clock();
        double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        int result = vm.ToInt(-1);
        vm.Pop();
        
        printf(" = %d (%.2f ms)\n", result, time);
        
        if (time > 1000.0) {
            printf("⚠️  Took > 1 second! Stopping.\n");
            break;
        }
    }
}

// ============================================
// TEST 4: Call Stack Overflow
// ============================================

void testCallStackOverflow() {
    printf("\n=== TEST 4: Call Stack Overflow ===\n");
    
    VM vm;
    
    // Função recursiva infinita: boom() { boom(); }
    Function* boomFunc = new Function("boom", 0);
    Chunk& chunk = boomFunc->chunk;
    
    int idx = chunk.addConstant(Value::makeString("boom"));
    chunk.write(OP_CALL, 1);
    chunk.write(idx, 1);
    chunk.write(0, 1);  // 0 args
    
    chunk.write(OP_NIL, 1);
    chunk.write(OP_RETURN, 1);
    
    uint16_t boomIdx = vm.registerFunction("boom", boomFunc);
    vm.Push(Value::makeFunction(boomIdx));
    vm.SetGlobal("boom");
    
    printf("Calling infinite recursion boom()...\n");
    
    vm.GetGlobal("boom");
    vm.Call(0, 0);  // Deve dar stack overflow!
    
    printf("✅ Survived (should have errored)\n");
}

// ============================================
// TEST 5: Type Confusion
// ============================================

void testTypeErrors() {
    printf("\n=== TEST 5: Type Errors ===\n");
    
    VM vm;
    
    // String + Int?
    printf("Test: string + int...\n");
    vm.PushString("hello");
    vm.PushInt(42);
    // Como fazer ADD sem bytecode? Precisa de função helper
    
    // Int como função?
    printf("Test: call int as function...\n");
    vm.PushInt(123);
    vm.Call(0, 0);  // Deve dar erro!
    
    // ToInt de string?
    printf("Test: ToInt of string...\n");
    vm.PushString("not a number");
    int n = vm.ToInt(-1);  // Deve dar erro!
    printf("Got: %d\n", n);
}

// ============================================
// TEST 6: Memory Stress
// ============================================

void testMemoryStress() {
    printf("\n=== TEST 6: Memory Stress (Strings) ===\n");
    
    VM vm;
    
    printf("Creating 10000 strings...\n");
    
    for (int i = 0; i < 10000; i++) {
        char buf[64];
        snprintf(buf, 64, "string_number_%d", i);
        
        vm.PushString(buf);
        vm.SetGlobal(buf);  // Guarda global
        
        if (i % 1000 == 0) {
            printf("  %d strings created...\n", i);
        }
    }
    
    printf("✅ Created 10000 strings\n");
    printf("⚠️  Memory leak? Check with valgrind!\n");
    
    // Limpa
    printf("Clearing stack...\n");
    while (vm.GetTop() > 0) {
        vm.Pop();
    }
}

// ============================================
// TEST 7: Invalid Indices
// ============================================

void testInvalidIndices() {
    printf("\n=== TEST 7: Invalid Stack Indices ===\n");
    
    VM vm;
    
    vm.PushInt(10);
    vm.PushInt(20);
    
    printf("Stack size: %d\n", vm.GetTop());
    
    // Acesso inválido
    printf("Test: Peek(999)...\n");
    const Value& v1 = vm.Peek(999);
    printf("Result type: %s\n", vm.TypeName(v1.type));
    
    printf("Test: Peek(-999)...\n");
    const Value& v2 = vm.Peek(-999);
    printf("Result type: %s\n", vm.TypeName(v2.type));
    
    printf("Test: ToInt(50)...\n");
    int n = vm.ToInt(50);
    printf("Result: %d\n", n);
}

// ============================================
// TEST 8: Global Overwrite
// ============================================

void testGlobalOverwrite() {
    printf("\n=== TEST 8: Global Overwrite ===\n");
    
    VM vm;
    
    // Set global várias vezes
    for (int i = 0; i < 100; i++) {
        vm.PushInt(i);
        vm.SetGlobal("x");
    }
    
    vm.GetGlobal("x");
    int result = vm.ToInt(-1);
    vm.Pop();
    
    printf("Final value of x: %d\n", result);
    
    if (result == 99) {
        printf("✅ Correctly overwrote\n");
    } else {
        printf("❌ Expected 99, got %d\n", result);
    }
    
    printf("⚠️  99 strings leaked? Need GC!\n");
}

// ============================================
// TEST 9: Nested Calls
// ============================================

void testNestedCalls() {
    printf("\n=== TEST 9: Nested Calls ===\n");
    
    VM vm;
    
    // a() calls b() calls c()
    Function* cFunc = new Function("c", 0);
    cFunc->chunk.write(OP_CONSTANT, 1);
    cFunc->chunk.write(cFunc->chunk.addConstant(Value::makeInt(42)), 1);
    cFunc->chunk.write(OP_RETURN, 1);
    
    Function* bFunc = new Function("b", 0);
    int idx = bFunc->chunk.addConstant(Value::makeString("c"));
    bFunc->chunk.write(OP_CALL, 1);
    bFunc->chunk.write(idx, 1);
    bFunc->chunk.write(0, 1);
    bFunc->chunk.write(OP_RETURN, 1);
    
    Function* aFunc = new Function("a", 0);
    idx = aFunc->chunk.addConstant(Value::makeString("b"));
    aFunc->chunk.write(OP_CALL, 1);
    aFunc->chunk.write(idx, 1);
    aFunc->chunk.write(0, 1);
    aFunc->chunk.write(OP_RETURN, 1);
    
    vm.registerFunction("c", cFunc);
    vm.registerFunction("b", bFunc);
    vm.registerFunction("a", aFunc);
    
    vm.Push(Value::makeFunction(vm.registerFunction("a", aFunc)));
    vm.SetGlobal("a");
    vm.Push(Value::makeFunction(vm.registerFunction("b", bFunc)));
    vm.SetGlobal("b");
    vm.Push(Value::makeFunction(vm.registerFunction("c", cFunc)));
    vm.SetGlobal("c");
    
    printf("Calling a() -> b() -> c()...\n");
    
    vm.GetGlobal("a");
    vm.Call(0, 1);
    
    int result = vm.ToInt(-1);
    printf("Result: %d\n", result);
    
    if (result == 42) {
        printf("✅ Nested calls work!\n");
    }
}

// ============================================
// TEST 10: Concurrent-like Stress
// ============================================

void testRapidFireCalls() {
    printf("\n=== TEST 10: Rapid Fire Calls ===\n");
    
    VM vm;
    
    // Simple add function
    Function* addFunc = new Function("add", 2);
    addFunc->chunk.write(OP_GET_LOCAL, 1);
    addFunc->chunk.write(0, 1);
    addFunc->chunk.write(OP_GET_LOCAL, 1);
    addFunc->chunk.write(1, 1);
    addFunc->chunk.write(OP_ADD, 1);
    addFunc->chunk.write(OP_RETURN, 1);
    
    uint16_t addIdx = vm.registerFunction("add", addFunc);
    vm.Push(Value::makeFunction(addIdx));
    vm.SetGlobal("add");
    
    printf("Calling add() 10000 times...\n");
    
    clock_t start = clock();
    
    for (int i = 0; i < 10000; i++) {
        vm.GetGlobal("add");
        vm.PushInt(i);
        vm.PushInt(i + 1);
        vm.Call(2, 1);
        vm.Pop();  // descarta resultado
    }
    
    clock_t end = clock();
    double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("✅ 10000 calls in %.2f ms\n", time);
    printf("   %.2f calls/ms\n", 10000.0 / time);
}

// ============================================
// MAIN - Roda Todos
// ============================================

// int main() {
//     printf("╔════════════════════════════════════╗\n");
//     printf("║   VM STRESS TESTS - REBENTA ISTO  ║\n");
//     printf("╚════════════════════════════════════╝\n");
    
//     testStackOverflow();
//     testStackUnderflow();
//     testDeepRecursion();
//     testCallStackOverflow();
//     testTypeErrors();
//     testMemoryStress();
//     testInvalidIndices();
//     testGlobalOverwrite();
//     testNestedCalls();
//     testRapidFireCalls();
    
//     printf("\n╔════════════════════════════════════╗\n");
//     printf("║       STRESS TESTS COMPLETE        ║\n");
//     printf("╚════════════════════════════════════╝\n");
//     printf("\nRun with valgrind to check leaks:\n");
//     printf("  valgrind --leak-check=full ./bin/stress_tests\n");
    
//     return 0;
// }

 

// ============================================
// BENCHMARK 1: Empty Call
// ============================================
void benchEmptyCall() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BENCHMARK: Empty Function Call   ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    VM vm;
    
    // Cria função vazia
    Function* emptyFunc = new Function("empty", 0);
    emptyFunc->chunk.write(OP_NIL, 1);
    emptyFunc->chunk.write(OP_RETURN, 1);
    
    uint16_t idx = vm.registerFunction("empty", emptyFunc);
    vm.Push(Value::makeFunction(idx));
    vm.SetGlobal("empty");
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < 10000; i++) {
        vm.GetGlobal("empty");
        vm.Call(0, 0);
    }
    
    // Benchmark
    const int CALLS = 100000;
    
    printf("Running %d calls...\n", CALLS);
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < CALLS; i++) {
        vm.GetGlobal("empty");
        vm.Call(0, 0);
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    double ms = duration.count() / 1000.0;
    double calls_per_ms = CALLS / ms;
    double calls_per_sec = calls_per_ms * 1000;
    
    printf("\n📊 Results:\n");
    printf("  Total time:  %.2f ms\n", ms);
    printf("  Calls/ms:    %.0f\n", calls_per_ms);
    printf("  Calls/sec:   %.0f\n", calls_per_sec);
    printf("  Time/call:   %.2f µs\n", duration.count() / (double)CALLS);
}

// ============================================
// BENCHMARK 2: Add Function
// ============================================
void benchAddFunction() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BENCHMARK: Add Function (a+b)    ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    VM vm;
    
    // Cria função add
    Function* addFunc = new Function("add", 2);
    addFunc->chunk.write(OP_GET_LOCAL, 1);
    addFunc->chunk.write(0, 1);
    addFunc->chunk.write(OP_GET_LOCAL, 1);
    addFunc->chunk.write(1, 1);
    addFunc->chunk.write(OP_ADD, 1);
    addFunc->chunk.write(OP_RETURN, 1);
    
    uint16_t idx = vm.registerFunction("add", addFunc);
    vm.Push(Value::makeFunction(idx));
    vm.SetGlobal("add");
    
    // Warmup
    printf("Warming up...\n");
    for (int i = 0; i < 10000; i++) {
        vm.GetGlobal("add");
        vm.PushInt(10);
        vm.PushInt(20);
        vm.Call(2, 1);
        vm.Pop();
    }
    
    // Benchmark
    const int CALLS = 50000;
    
    printf("Running %d calls...\n", CALLS);
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < CALLS; i++) {
        vm.GetGlobal("add");
        vm.PushInt(i);
        vm.PushInt(i + 1);
        vm.Call(2, 1);
        vm.Pop();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    double ms = duration.count() / 1000.0;
    double calls_per_ms = CALLS / ms;
    double calls_per_sec = calls_per_ms * 1000;
    
    printf("\n📊 Results:\n");
    printf("  Total time:  %.2f ms\n", ms);
    printf("  Calls/ms:    %.0f\n", calls_per_ms);
    printf("  Calls/sec:   %.0f\n", calls_per_sec);
    printf("  Time/call:   %.2f µs\n", duration.count() / (double)CALLS);
}

// ============================================
// BENCHMARK 3: Fibonacci
// ============================================
void benchFibonacci() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BENCHMARK: Fibonacci (fib)       ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    VM vm;
    
    // Cria função fib
    Function* fibFunc = new Function("fib", 1);
    Chunk& chunk = fibFunc->chunk;
    
    // if (n < 2) return n;
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    int idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_LESS, 1);
    chunk.write(OP_JUMP_IF_FALSE, 1);
    int elseJump = chunk.count();
    chunk.write(0, 1);
    chunk.write(0, 1);
    chunk.write(OP_POP, 1);
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    chunk.write(OP_RETURN, 1);
    
    int offset = chunk.count() - elseJump - 2;
    chunk.code[elseJump] = (offset >> 8) & 0xff;
    chunk.code[elseJump + 1] = offset & 0xff;
    chunk.write(OP_POP, 1);
    
    // return fib(n-1) + fib(n-2);
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(1));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    idx = chunk.addConstant(Value::makeString("fib"));
    chunk.write(OP_CALL, 1);
    chunk.write(idx, 1);
    chunk.write(1, 1);
    
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    idx = chunk.addConstant(Value::makeString("fib"));
    chunk.write(OP_CALL, 1);
    chunk.write(idx, 1);
    chunk.write(1, 1);
    
    chunk.write(OP_ADD, 1);
    chunk.write(OP_RETURN, 1);
    
    uint16_t fibIdx = vm.registerFunction("fib", fibFunc);
    vm.Push(Value::makeFunction(fibIdx));
    vm.SetGlobal("fib");
    
    // Benchmark
    int testValues[] = {20, 25, 30};
    
    for (int n : testValues) {
        printf("\n--- fib(%d) ---\n", n);
        
        auto start = high_resolution_clock::now();
        
        vm.GetGlobal("fib");
        vm.PushInt(n);
        vm.Call(1, 1);
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        int result = vm.ToInt(-1);
        vm.Pop();
        
        double ms = duration.count() / 1000.0;
        
        printf("Result: %d\n", result);
        printf("Time:   %.2f ms\n", ms);
    }
}

// ============================================
// BENCHMARK 4: Stack Operations
// ============================================
void benchStackOps() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BENCHMARK: Stack Operations      ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    VM vm;
    
    const int OPS = 1000000;
    
    printf("Running %d operations (push+peek+pop)...\n", OPS);
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < OPS; i++) {
        vm.PushInt(i);
        vm.Peek(-1);
        vm.Pop();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    double ms = duration.count() / 1000.0;
    double ops_per_ms = OPS / ms;
    double ops_per_sec = ops_per_ms * 1000;
    
    printf("\n📊 Results:\n");
    printf("  Total time:  %.2f ms\n", ms);
    printf("  Ops/ms:      %.0f\n", ops_per_ms);
    printf("  Ops/sec:     %.0f (%.1fM/sec)\n", ops_per_sec, ops_per_sec/1000000.0);
}

// ============================================
// MAIN
// ============================================
int main() {
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║       VM PERFORMANCE BENCHMARKS                ║\n");
    printf("║   High-precision timing (std::chrono)         ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    benchEmptyCall();
    benchAddFunction();
    benchFibonacci();
    benchStackOps();
    
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║            BENCHMARKS COMPLETE                 ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    printf("\n📊 Comparison:\n");
    printf("  Lua 5.4:     57M empty calls/sec\n");
    printf("  Python 3.12: 20M empty calls/sec\n");
    printf("  Your VM:     Check results above!\n");
    
    return 0;
}
