#include "chunk.h"
#include "vm.h"
#include "stringpool.h" 
#include "debug.h"
#include <cstdio>
#include <ctime>
#include <chrono>


using namespace std::chrono;




// ============================================
// BENCHMARK 1: Empty Call (SEM MUDANÇAS)
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
// BENCHMARK 2: Add Function (SEM MUDANÇAS)
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
// BENCHMARK 3: Fibonacci (✅ CORRIGIDO!)
// ============================================
void benchFibonacci() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BENCHMARK: Fibonacci (fib)       ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    VM vm;
    
    // Cria função fib
    Function* fibFunc = new Function("fib", 1);
    Chunk& chunk = fibFunc->chunk;
    
    // ✅ IMPORTANTE: Registra função PRIMEIRO para pegar índice
    uint16_t fibIdx = vm.registerFunction("fib", fibFunc);
    
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
    
    // ✅ MUDANÇA: return fib(n-1) + fib(n-2);
    // Primeira chamada: fib(n-1)
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(1));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    
    // ✅ OP_CALL com índice direto (não string!)
    chunk.write(OP_CALL, 1);
    chunk.write((fibIdx >> 8) & 0xFF, 1);  // High byte
    chunk.write(fibIdx & 0xFF, 1);         // Low byte
    chunk.write(1, 1);                     // argCount
    
    // Segunda chamada: fib(n-2)
    chunk.write(OP_GET_LOCAL, 1);
    chunk.write(0, 1);
    idx = chunk.addConstant(Value::makeInt(2));
    chunk.write(OP_CONSTANT, 1);
    chunk.write(idx, 1);
    chunk.write(OP_SUBTRACT, 1);
    
    // ✅ OP_CALL com índice direto (não string!)
    chunk.write(OP_CALL, 1);
    chunk.write((fibIdx >> 8) & 0xFF, 1);  // High byte
    chunk.write(fibIdx & 0xFF, 1);         // Low byte
    chunk.write(1, 1);                     // argCount
    
    chunk.write(OP_ADD, 1);
    chunk.write(OP_RETURN, 1);
    
    // Define global
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
// BENCHMARK 4: Stack Operations (SEM MUDANÇAS)
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
// String Interning Test (SEM MUDANÇAS)
// ============================================
void testStringInterning() {
    printf("\n=== String Interning Test ===\n");
    
    StringPool::instance().clear();
    
    clock_t start = clock();
    
    for (int i = 0; i < 100000; i++) {
        Value v = Value::makeString("hello");
    }
    
    clock_t end = clock();
    double ms = (end - start) * 1000.0 / CLOCKS_PER_SEC;
    
    printf("Created 100000 strings in %.2f ms\n", ms);
    printf("Pool size: %zu (should be 1!)\n", 
           StringPool::instance().size());
    
    StringPool::instance().dumpStats();
    
    Value v1 = Value::makeString("test");
    Value v2 = Value::makeString("test");
    
    printf("\nComparison test:\n");
    printf("  v1.stringPtr: %p\n", v1.as.stringPtr);
    printf("  v2.stringPtr: %p\n", v2.as.stringPtr);
    printf("  Same pointer: %s\n", 
           v1.as.stringPtr == v2.as.stringPtr ? "YES ✅" : "NO ❌");
}

// ============================================
// MAIN
// ============================================
int main() {
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║       VM PERFORMANCE BENCHMARKS                ║\n");
    printf("║   High-precision timing (std::chrono)         ║\n");
    printf("║   ✅ Using direct function indices (fast!)    ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    benchEmptyCall();
    benchAddFunction();
    benchFibonacci();
    benchStackOps();
    testStringInterning();
    
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║            BENCHMARKS COMPLETE                 ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    printf("\n📊 Comparison:\n");
    printf("  Lua 5.4:     57M empty calls/sec\n");
    printf("  Python 3.12: 20M empty calls/sec\n");
    printf("  Your VM:     Check results above!\n");
    printf("  (Now with optimized function calls!) 🚀\n");
    
    return 0;
}
