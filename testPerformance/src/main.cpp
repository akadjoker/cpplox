#include "compiler.h"
#include "vm.h"
// benchmark.cpp
#include "vm.h"
#include "compiler.h"
#include <chrono>
#include <iostream>

class Benchmark
{
public:
    static void run(const std::string &name, const std::string &code, int iterations = 1)
    {
        VM vm;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; i++)
        {
            vm.interpret(code);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        double ms = duration.count() / 1000.0;
        double avg_ms = ms / iterations;

        std::cout << "┌─────────────────────────────────────┐\n";
        std::cout << "│ " << name << "\n";
        std::cout << "├─────────────────────────────────────┤\n";
        std::cout << "│ Total:   " << ms << " ms\n";
        std::cout << "│ Average: " << avg_ms << " ms\n";
        std::cout << "│ Iterations: " << iterations << "\n";
        std::cout << "└─────────────────────────────────────┘\n\n";
    }
};

int main()
{
    std::cout << "\n🔥 PERFORMANCE BENCHMARKS 🔥\n\n";

    // ============================================
    // 1. FIBONACCI RECURSIVO
    // ============================================
    {
        std::string code = R"(
            def fib(n) {
                if (n <= 1) {
                    return n;
                }
                return fib(n - 1) + fib(n - 2);
            }
            var result = fib(30);
        )";

        Benchmark::run("Fibonacci(30) - Recursivo", code);
    }

    // ============================================
    // 2. FIBONACCI ITERATIVO
    // ============================================
    {
        std::string code = R"(
            def fib(n) {
                var a = 0;
                var b = 1;
                for (var i = 0; i < n; i++) {
                    var temp = a;
                    a = b;
                    b = temp + b;
                }
                return a;
            }
            var result = fib(1000);
        )";

        Benchmark::run("Fibonacci(1000) - Iterativo", code);
    }

    // ============================================
    // 3. FACTORIAL
    // ============================================
    {
        std::string code = R"(
            def factorial(n) {
                if (n <= 1) {
                    return 1;
                }
                return n * factorial(n - 1);
            }
            var result = factorial(20);
        )";

        Benchmark::run("Factorial(20) - Recursivo", code);
    }

    // ============================================
    // 4. LOOP INTENSIVO
    // ============================================
    {
        std::string code = R"(
            var sum = 0;
            for (var i = 0; i < 1000000; i++) {
                sum += i;
            }
        )";

        Benchmark::run("Loop 1M iterations", code);
    }

    // ============================================
    // 5. NESTED LOOPS
    // ============================================
    {
        std::string code = R"(
            var total = 0;
            for (var i = 0; i < 100; i++) {
                for (var j = 0; j < 100; j++) {
                    total += i * j;
                }
            }
        )";

        Benchmark::run("Nested Loops (100x100)", code);
    }

    // ============================================
    // 6. ACKERMANN (stress test)
    // ============================================
    {
        std::string code = R"(
            def ackermann(m, n) {
                if (m == 0) {
                    return n + 1;
                }
                if (n == 0) {
                    return ackermann(m - 1, 1);
                }
                return ackermann(m - 1, ackermann(m, n - 1));
            }
            var result = ackermann(3, 6);
        )";

        Benchmark::run("Ackermann(3, 6)", code);
    }

    // ============================================
    // 7. PRIME NUMBERS
    // ============================================
    {
        std::string code = R"(
            def isPrime(n) {
                if (n < 2) {
                    return false;
                }
                for (var i = 2; i * i <= n; i++) {
                    if (n % i == 0) {
                        return false;
                    }
                }
                return true;
            }
            
            var count = 0;
            for (var i = 0; i < 10000; i++) {
                if (isPrime(i)) {
                    count++;
                }
            }
        )";

        Benchmark::run("Count Primes < 10000", code);
    }

    // ============================================
    // 8. STRING OPERATIONS
    // ============================================
    {
        std::string code = R"(
            var result = "";
            for (var i = 0; i < 1000; i++) {
                result = result + "x";
            }
        )";

        Benchmark::run("String Concatenation (1000x)", code);
    }

    // ============================================
    // 9. ARRAY OPERATIONS
    // ============================================
    {
        std::string code = R"(
           // Fibonacci recursivo
            def fib(n) {
                if (n <= 1) {
                    return n;
                }
                return fib(n - 1) + fib(n - 2);
            }

            // Benchmark
            print("=== Fibonacci Benchmark ===");

            var start = clock();
            var result = fib(30);
            var end = clock();

            var elapsed = (end - start) * 1000;  // Convert to ms

            print("Result: ");
            print(result);
            print("Time: ");
            print(elapsed);
            print(" ms");
        )";

        Benchmark::run(" Fibonacci", code);
    }

    return 0;
}