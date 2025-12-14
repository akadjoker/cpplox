#include "compiler.h"
#include "vm.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <type_traits>

// ============================================
// TEST FRAMEWORK
// ============================================

int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

#define TEST(name)                                                 \
    void test_##name();                                            \
    struct TestRegistrar_##name                                    \
    {                                                              \
        TestRegistrar_##name()                                     \
        {                                                          \
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"; \
            std::cout << "🧪 TEST: " << #name << "\n";             \
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";   \
            totalTests++;                                          \
            try                                                    \
            {                                                      \
                test_##name();                                     \
                passedTests++;                                     \
                std::cout << "✅ PASSED\n";                        \
            }                                                      \
            catch (const std::exception &e)                        \
            {                                                      \
                failedTests++;                                     \
                std::cout << "❌ FAILED: " << e.what() << "\n";    \
            }                                                      \
        }                                                          \
    } testRegistrar_##name;                                        \
    void test_##name()

// Helper for converting values to string
template <typename T>
std::string valueToString(const T &val)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return "\"" + val + "\"";
    }
    else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
    {
        return std::string("\"") + val + "\"";
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return val ? "true" : "false";
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        return std::to_string(val);
    }
    else
    {
        return "<unknown>";
    }
}

#define ASSERT_EQ(a, b)                                                \
    do                                                                 \
    {                                                                  \
        auto val_a = (a);                                              \
        auto val_b = (b);                                              \
        if (val_a != val_b)                                            \
        {                                                              \
            throw std::runtime_error(                                  \
                std::string("Assertion failed: ") + #a + " == " + #b + \
                "\n  Expected: " + valueToString(val_b) +              \
                "\n  Got:      " + valueToString(val_a));              \
        }                                                              \
    } while (0)

#define ASSERT_DOUBLE_EQ(a, b)                                        \
    do                                                                \
    {                                                                 \
        double val_a = (a);                                           \
        double val_b = (b);                                           \
        if (std::abs(val_a - val_b) > 0.0001)                         \
        {                                                             \
            throw std::runtime_error(                                 \
                std::string("Assertion failed: ") + #a + " ≈ " + #b + \
                "\n  Expected: " + std::to_string(val_b) +            \
                "\n  Got:      " + std::to_string(val_a));            \
        }                                                             \
    } while (0)

#define ASSERT_TRUE(cond)                                                \
    do                                                                   \
    {                                                                    \
        if (!(cond))                                                     \
        {                                                                \
            throw std::runtime_error(                                    \
                std::string("Assertion failed: ") + #cond + " is true"); \
        }                                                                \
    } while (0)

#define ASSERT_FALSE(cond)                                                \
    do                                                                   \
    {                                                                    \
        if (cond)                                                        \
        {                                                                \
            throw std::runtime_error(                                    \
                std::string("Assertion failed: ") + #cond + " is false"); \
        }                                                                \
    } while (0)


#define ASSERT_NEAR(actual, expected, epsilon)                                       \
    do                                                                               \
    {                                                                                \
        if (std::abs((actual) - (expected)) > (epsilon))                             \
        {                                                                            \
            printf("❌ FAILED: Assertion failed: %s near %s\n", #actual, #expected); \
            printf("  Expected: %.10f\n", (double)(expected));                       \
            printf("  Got:      %.10f\n", (double)(actual));                         \
            printf("  Diff:     %.10f (max allowed: %.10f)\n",                       \
                   std::abs((actual) - (expected)), (double)(epsilon));              \
            return;                                                                  \
        }                                                                            \
    } while (0)

Value executeExpression(const std::string &code)
{
    VM vm;

    InterpretResult result = vm.interpretExpression(code);

    if (result != InterpretResult::OK)
    {

        throw std::runtime_error("Runtime error: " + code);
    }

    Value ret = vm.Pop();

    return ret;
}

Value executeProgram(const std::string &code)
{
    VM vm;
    InterpretResult result = vm.interpret(code);

    if (result != InterpretResult::OK)
    {
        throw std::runtime_error("Runtime error: " + code);
    }

    Value ret = vm.Pop();
    printValue(ret);
    return ret;
}
Value executeProgram(const std::string &code, const std::string &varName)
{
    VM vm;
    InterpretResult result = vm.interpret(code);
    if (result != InterpretResult::OK)
    {
        throw std::runtime_error("Runtime error: " + code);
    }
    vm.GetGlobal(varName.c_str()); // push para stack
    return vm.Pop();               // pop e devolve
}

TEST(function_with_return)
{
    std::string code = R"(
        def add(a, b) { return a + b; }
        var result = add(10, 5);
 
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 15);
}

TEST(function_no_return)
{
    std::string code = R"(
        def sayHello() { print("Hello!"); }
        var result = sayHello();
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.isNull());
}

TEST(function_with_multiple_params)
{
    std::string code = R"(
        def multiply(a, b, c) { return a * b * c; }
        var result = multiply(2, 3, 4);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 24);
}

TEST(function_mutual_recursion)
{
    std::string code = R"(
        def isEven(n) {
            if (n == 0) return true;
            return isOdd(n - 1);
        }
        def isOdd(n) {
            if (n == 0) return false;
            return isEven(n - 1);
        }
        var result = isEven(6);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(if_statement_true_branch)
{
    std::string code = R"(
        var x = 10;
        if (x > 5) {
            x = 20;
        }
        var result = x;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 20);
}

TEST(if_statement_false_branch)
{
    std::string code = R"(
        var x = 3;
        if (x > 5) {
            x = 20;
        }
        var result = x;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 3);
}

TEST(if_else_statement_true)
{
    std::string code = R"(
        var x = 10;
        if (x > 5) {
            x = 20;
        } else {
            x = 30;
        }
        var result = x;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 20);
}

TEST(if_else_statement_false)
{
    std::string code = R"(
        var x = 3;
        if (x > 5) {
            x = 20;
        } else {
            x = 30;
        }
        var result = x;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 30);
}

TEST(if_nested)
{
    std::string code = R"(
        var x = 10;
        var y = 5;
        if (x > 5) {
            if (y > 3) {
                x = 100;
            }
        }
        var result = x;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 100);
}

TEST(if_with_expressions)
{
    std::string code = R"(
        var a = 5;
        var b = 3;
        var result = 0;
        if (a + b > 7) {
            result = 10;
        } else {
            result = 20;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 10);
}

TEST(if_elif_first_true)
{
    std::string code = R"(
        var x = 10;
        var result = 0;
        if (x < 5) {
            result = 1;
        } elif (x < 15) {
            result = 2;
        } else {
            result = 3;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 2);
}

TEST(if_elif_second_true)
{
    std::string code = R"(
        var x = 20;
        var result = 0;
        if (x < 5) {
            result = 1;
        } elif (x < 15) {
            result = 2;
        } elif (x < 25) {
            result = 3;
        } else {
            result = 4;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 3);
}

TEST(if_elif_else_fallthrough)
{
    std::string code = R"(
        var x = 100;
        var result = 0;
        if (x < 5) {
            result = 1;
        } elif (x < 15) {
            result = 2;
        } elif (x < 25) {
            result = 3;
        } else {
            result = 4;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 4);
}

TEST(if_elif_multiple)
{
    std::string code = R"(
        var score = 75;
        var grade = "";
        if (score >= 90) {
            grade = "A";
        } elif (score >= 80) {
            grade = "B";
        } elif (score >= 70) {
            grade = "C";
        } elif (score >= 60) {
            grade = "D";
        } else {
            grade = "F";
        }
    )";

    VM vm;
    vm.interpret(code);
    vm.GetGlobal("grade");
    ASSERT_EQ(std::string(vm.Pop().asString()), "C");
}

TEST(if_elif_no_else)
{
    std::string code = R"(
        var x = 100;
        var result = 0;
        if (x < 5) {
            result = 1;
        } elif (x < 15) {
            result = 2;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 0); // Nenhuma condição verdadeira
}

TEST(if_elif_with_expressions)
{
    std::string code = R"(
        var a = 5;
        var b = 3;
        var result = 0;
        if (a + b > 10) {
            result = 1;
        } elif (a * b > 10) {
            result = 2;
        } elif (a - b > 0) {
            result = 3;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 2); // 5 * 3 = 15 > 10
}

TEST(nested_if_elif)
{
    std::string code = R"(
        var x = 10;
        var y = 5;
        var result = 0;
        if (x > 5) {
            if (y > 3) {
                result = 1;
            } elif (y > 2) {
                result = 2;
            }
        } elif (x > 0) {
            result = 3;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 1);
}

TEST(test_while)
{
    std::string code = R"(
        var i = 0;
        while (i < 5) {
            
            i = i + 1;
        }
    )";
    Value result = executeProgram(code, "i");
    ASSERT_EQ(result.asInt(), 5);
}
TEST(while_with_break)
{
    std::string code = R"(
        var i = 0;
        while (i < 10) {
            if (i == 5) {
                break;
            }
            i = i + 1;
        }
    )";
    Value result = executeProgram(code, "i");
    ASSERT_EQ(result.asInt(), 5);
}

TEST(while_with_continue)
{
    std::string code = R"(
        var sum = 0;
        var i = 0;
        while (i < 10) {
            i = i + 1;
            if (i == 5) {
                continue;
            }
            sum = sum + i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 50); // 1+2+3+4+6+7+8+9+10 = 50 (sem o 5)
}

TEST(function_with_if_statement)
{
    std::string code = R"(
        def max(a, b) {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
        var result = max(10, 5);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 10);
}

TEST(function_with_elif)
{
    std::string code = R"(
        def classify(n) {
            if (n < 0) {
                return -1;
            } elif (n == 0) {
                return 0;
            } else {
                return 1;
            }
        }
        var r1 = classify(-5);
        var r2 = classify(0);
        var r3 = classify(10);
    )";

    VM vm;
    vm.interpret(code);

    vm.GetGlobal("r1");
    ASSERT_EQ(vm.Pop().asInt(), -1);

    vm.GetGlobal("r2");
    ASSERT_EQ(vm.Pop().asInt(), 0);

    vm.GetGlobal("r3");
    ASSERT_EQ(vm.Pop().asInt(), 1);
}

TEST(function_with_while_loop)
{
    std::string code = R"(
        def sum(n) {
            var total = 0;
            var i = 1;
            while (i <= n) {
                total = total + i;
                i = i + 1;
            }
            return total;
        }
        var result = sum(10);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 55); // 1+2+3+...+10 = 55
}

TEST(function_with_while_and_break)
{
    std::string code = R"(
        def findFirst(limit) {
            var i = 0;
            while (i < limit) {
                if (i == 7) {
                    break;
                }
                i = i + 1;
            }
            return i;
        }
        var result = findFirst(20);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 7);
}

TEST(function_with_while_and_continue)
{
    std::string code = R"(
        def sumOdds(n) {
            var sum = 0;
            var i = 0;
            while (i < n) {
                i = i + 1;
                if (i % 2 == 0) {
                    continue;
                }
                sum = sum + i;
            }
            return sum;
        }
        var result = sumOdds(10);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 25); // 1+3+5+7+9 = 25
}

TEST(function_with_nested_loops_small)
{
    std::string code = R"(
        def multiplyTable(n) {
            var sum = 0;
            var i = 1;
            while (i <= n) 
            {
                var j = 1;
                while (j <= n) {
                    sum = sum + (i * j);
                    j = j + 1;
                }
                i = i + 1;
            }
            return sum;
        }
        var result = multiplyTable(2);  // ← 2 em vez de 3!
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 9); // (1+2) + (2+4) = 3 + 6 = 9
}

TEST(function_with_nested_loops)
{
    std::string code = R"(
        def multiplyTable(n) {
            var sum = 0;
            var i = 1;
            while (i <= n) {
                var j = 1;
                while (j <= n) {
                    sum = sum + (i * j);
                    j = j + 1;
                }
                i = i + 1;
            }
            return sum;
        }
        var result = multiplyTable(3);
    )";
    Value result = executeProgram(code, "result");
    // (1*1 + 1*2 + 1*3) + (2*1 + 2*2 + 2*3) + (3*1 + 3*2 + 3*3)
    // (1 + 2 + 3) + (2 + 4 + 6) + (3 + 6 + 9) = 6 + 12 + 18 = 36
    ASSERT_EQ(result.asInt(), 36);
}

TEST(if_with_modulo)
{
    std::string code = R"(
        var x = 10;
        var result = 0;
        if (x % 2 == 0) {
            result = 1;
        } else {
            result = 2;
        }
    )";

    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 1);
}

// ============================================
// TESTES DE OPERADORES ARITMÉTICOS
// ============================================

TEST(arithmetic_addition_int)
{
    std::string code = R"(
        var result = 10 + 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 15);
}

TEST(arithmetic_addition_double)
{
    std::string code = R"(
        var result = 10.5 + 5.3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asDouble(), 15.8);
}

TEST(arithmetic_addition_mixed)
{
    std::string code = R"(
        var result = 10 + 5.5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asDouble(), 15.5);
}

TEST(arithmetic_subtraction_int)
{
    std::string code = R"(
        var result = 10 - 3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 7);
}

TEST(arithmetic_subtraction_double)
{
    std::string code = R"(
        var result = 10.5 - 3.2;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_NEAR(result.asDouble(), 7.3, 0.0001);
}

TEST(arithmetic_multiplication_int)
{
    std::string code = R"(
        var result = 7 * 6;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 42);
}

TEST(arithmetic_multiplication_double)
{
    std::string code = R"(
        var result = 2.5 * 4.0;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asDouble(), 10.0);
}

TEST(arithmetic_division_int)
{
    std::string code = R"(
        var result = 20 / 4;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 5);
}

TEST(arithmetic_division_double)
{
    std::string code = R"(
        var result = 15.0 / 3.0;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asDouble(), 5.0);
}

TEST(arithmetic_division_int_truncates)
{
    std::string code = R"(
        var result = 7 / 2;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 3); // Integer division
}

TEST(arithmetic_modulo_basic)
{
    std::string code = R"(
        var result = 10 % 3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 1);
}

TEST(arithmetic_modulo_even)
{
    std::string code = R"(
        var result = 10 % 2;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 0);
}

TEST(arithmetic_modulo_larger_divisor)
{
    std::string code = R"(
        var result = 5 % 10;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 5);
}

TEST(arithmetic_negate_int)
{
    std::string code = R"(
        var result = -42;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), -42);
}

TEST(arithmetic_negate_double)
{
    std::string code = R"(
        var result = -3.14;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asDouble(), -3.14);
}

TEST(arithmetic_negate_expression)
{
    std::string code = R"(
        var result = -(10 + 5);
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), -15);
}

// ============================================
// TESTES DE PRECEDÊNCIA
// ============================================

TEST(precedence_multiplication_before_addition)
{
    std::string code = R"(
        var result = 2 + 3 * 4;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 14); // 2 + (3 * 4) = 14, not (2 + 3) * 4 = 20
}

TEST(precedence_division_before_subtraction)
{
    std::string code = R"(
        var result = 20 - 10 / 2;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 15); // 20 - (10 / 2) = 15
}

TEST(precedence_parentheses_override)
{
    std::string code = R"(
        var result = (2 + 3) * 4;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 20);
}

TEST(precedence_complex_expression)
{
    std::string code = R"(
        var result = 2 + 3 * 4 - 10 / 2;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 9); // 2 + 12 - 5 = 9
}

TEST(precedence_modulo_same_as_multiply)
{
    std::string code = R"(
        var result = 10 + 7 % 3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 11); // 10 + (7 % 3) = 10 + 1 = 11
}

// ============================================
// TESTES DE OPERADORES DE COMPARAÇÃO
// ============================================

TEST(comparison_equal_int_true)
{
    std::string code = R"(
        var result = 5 == 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_equal_int_false)
{
    std::string code = R"(
        var result = 5 == 3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_not_equal_true)
{
    std::string code = R"(
        var result = 5 != 3;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_not_equal_false)
{
    std::string code = R"(
        var result = 5 != 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_greater_true)
{
    std::string code = R"(
        var result = 10 > 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_greater_false)
{
    std::string code = R"(
        var result = 5 > 10;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_greater_equal_true)
{
    std::string code = R"(
        var result = 10 >= 10;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_greater_equal_false)
{
    std::string code = R"(
        var result = 5 >= 10;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_less_true)
{
    std::string code = R"(
        var result = 5 < 10;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_less_false)
{
    std::string code = R"(
        var result = 10 < 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_less_equal_true)
{
    std::string code = R"(
        var result = 5 <= 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_less_equal_false)
{
    std::string code = R"(
        var result = 10 <= 5;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_string_equal_true)
{
    std::string code = R"(
        var result = "hello" == "hello";
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(comparison_string_equal_false)
{
    std::string code = R"(
        var result = "hello" == "world";
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_different_types_false)
{
    std::string code = R"(
        var result = 5 == "5";
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(comparison_null_equality)
{
    std::string code = R"(
        var a = nil;
        var b = nil;
        var result = a == b;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

// ============================================
// TESTES DE OPERADORES LÓGICOS
// ============================================

TEST(logical_not_true)
{
    std::string code = R"(
        var result = !false;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(logical_not_false)
{
    std::string code = R"(
        var result = !true;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(logical_and_both_true)
{
    std::string code = R"(
        var result = true && true;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(logical_and_first_false)
{
    std::string code = R"(
        var result = false && true;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(logical_and_second_false)
{
    std::string code = R"(
        var result = true && false;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(logical_and_short_circuit)
{
    std::string code = R"(
        var x = 0;
        var result = false && (x = 1);
    )";

    VM vm;
    vm.interpret(code);

    vm.GetGlobal( "x");
    ASSERT_EQ(vm.Pop().asInt(), 0); // x não deve ser modificado
}

TEST(logical_or_both_false)
{
    std::string code = R"(
        var result = false || false;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_FALSE(result.asBool());
}

TEST(logical_or_first_true)
{
    std::string code = R"(
        var result = true || false;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(logical_or_second_true)
{
    std::string code = R"(
        var result = false || true;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool());
}

TEST(logical_or_short_circuit)
{
    std::string code = R"(
        var x = 0;
        var result = true || (x = 1);
    )";

    VM vm;
    vm.interpret(code);

    vm.GetGlobal("x");
    ASSERT_EQ(vm.Pop().asInt(), 0); // x não deve ser modificado
}

TEST(logical_combined_and_or)
{
    std::string code = R"(
        var result = true && false || true;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_TRUE(result.asBool()); // (true && false) || true = false || true = true
}

// ============================================
// TESTES DE STRING CONCATENATION
// ============================================

TEST(string_concatenation_basic)
{
    std::string code = R"(
        var result = "Hello, " + "World!";
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(std::string(result.asString()), "Hello, World!");
}

TEST(string_concatenation_multiple)
{
    std::string code = R"(
        var result = "a" + "b" + "c";
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(std::string(result.asString()), "abc");
}

TEST(string_concatenation_with_variables)
{
    std::string code = R"(
        var a = "Hello";
        var b = "World";
        var result = a + " " + b;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(std::string(result.asString()), "Hello World");
}


TEST(for_loop_basic)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 1; i <= 5; i = i + 1) {
            sum = sum + i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 15);  // 1+2+3+4+5
}


TEST(for_loop_no_initializer)
{
    std::string code = R"(
        var i = 0;
        var sum = 0;
        for (; i < 5; i = i + 1) {
            sum = sum + i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 10);  // 0+1+2+3+4
}

TEST(for_loop_no_condition)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 0; ; i = i + 1) {
            sum = sum + i;
            if (i >= 5) {
                break;
            }
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 15);  // 0+1+2+3+4+5
}

TEST(for_loop_no_increment)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 0; i < 5; ) {
            sum = sum + i;
            i = i + 1;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 10);  // 0+1+2+3+4
}

TEST(for_loop_with_break)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 0; i < 10; i = i + 1) {
            if (i == 5) {
                break;
            }
            sum = sum + i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 10);  // 0+1+2+3+4
}

TEST(for_loop_with_continue)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 0; i < 10; i = i + 1) {
            if (i % 2 == 0) {
                continue;
            }
            sum = sum + i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 25);  // 1+3+5+7+9
}

TEST(for_loop_nested)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 1; i <= 3; i = i + 1) {
            for (var j = 1; j <= 3; j = j + 1) {
                sum = sum + (i * j);
            }
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 36);  // (1+2+3) + (2+4+6) + (3+6+9)
}

TEST(for_loop_scope_isolation)
{
    std::string code = R"(
        for (var i = 0; i < 3; i++) {
            var x = i * 10;
        }
        // i e x não existem aqui - não dá erro mas também não são acessíveis
        var result = 42;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 42);
}


TEST(compound_assignment_add)
{
    std::string code = R"(
        var x = 10;
        x += 5;
    )";
    Value result = executeProgram(code, "x");
    ASSERT_EQ(result.asInt(), 15);
}

TEST(prefix_increment)
{
    std::string code = R"(
        var i = 5;
        var result = ++i;
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal( "i");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal( "result");
    ASSERT_EQ(vm.Pop().asInt(), 6);  // Retorna novo valor
}

TEST(postfix_increment)
{
    std::string code = R"(
        var i = 5;
        var result = i++;
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("i");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("result");
    ASSERT_EQ(vm.Pop().asInt(), 5);  // Retorna valor antigo
}


// ============================================
// TESTES DE OPERADORES COM LOOPS
// ============================================

TEST(operators_increment_in_loop)
{
    std::string code = R"(
        var sum = 0;
        for (var i = 0; i < 10; i++) {
            sum += i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 45);  // 0+1+2+...+9
}

TEST(operators_compound_in_while)
{
    std::string code = R"(
        var x = 1;
        var count = 0;
        while (x < 100) {
            x *= 2;
            count++;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("x");
    ASSERT_EQ(vm.Pop().asInt(), 128);  // 1*2*2*2*2*2*2*2 = 128
    
    vm.GetGlobal("count");
    ASSERT_EQ(vm.Pop().asInt(), 7);
}

TEST(operators_modulo_in_loop)
{
    std::string code = R"(
        var evens = 0;
        var odds = 0;
        for (var i = 0; i < 20; i++) {
            if (i % 2 == 0) {
                evens++;
            } else {
                odds++;
            }
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("evens");
    ASSERT_EQ(vm.Pop().asInt(), 10);
    
    vm.GetGlobal("odds");
    ASSERT_EQ(vm.Pop().asInt(), 10);
}

TEST(operators_division_accumulate)
{
    std::string code = R"(
        var x = 1000;
        var steps = 0;
        while (x > 1) {
            x /= 2;
            steps++;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("x");
    ASSERT_EQ(vm.Pop().asInt(),     1);  // 1000/2/2/.../2 eventually reaches 0 (int division)
    
    vm.GetGlobal("steps");
    ASSERT_EQ(vm.Pop().asInt(), 9);  // Takes 10 divisions to go from 1000 to 0
}

TEST(operators_prefix_in_condition)
{
    std::string code = R"(
        var i = 0;
        var sum = 0;
        while (++i <= 5) {
            sum += i;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("i");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("sum");
    ASSERT_EQ(vm.Pop().asInt(), 15);  // 1+2+3+4+5
}

TEST(operators_postfix_in_array_style)
{
    std::string code = R"(
        var index = 0;
        var sum = 0;
        
        // Simula processar 5 elementos
        sum += index++;  // 0
        sum += index++;  // 1
        sum += index++;  // 2
        sum += index++;  // 3
        sum += index++;  // 4
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("index");
    ASSERT_EQ(vm.Pop().asInt(), 5);
    
    vm.GetGlobal("sum");
    ASSERT_EQ(vm.Pop().asInt(), 10);  // 0+1+2+3+4
}

TEST(operators_mixed_compound_in_loop)
{
    std::string code = R"(
        var a = 10;
        var b = 2;
        
        for (var i = 0; i < 3; i++) {
            a += 5;   // 15, 20, 25
            b *= 2;   // 4, 8, 16
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("a");
    ASSERT_EQ(vm.Pop().asInt(), 25);
    
    vm.GetGlobal("b");
    ASSERT_EQ(vm.Pop().asInt(), 16);
}

TEST(operators_nested_loops_with_increment)
{
    std::string code = R"(
        var total = 0;
        for (var i = 0; i < 3; i++) {
            for (var j = 0; j < 3; j++) {
                total++;
            }
        }
    )";
    Value result = executeProgram(code, "total");
    ASSERT_EQ(result.asInt(), 9);  // 3x3
}

TEST(operators_fibonacci_with_compound)
{
    std::string code = R"(
        var a = 0;
        var b = 1;
        var count = 0;
        
        while (count < 10) {
            var temp = a;
            a = b;
            b += temp;
            count++;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("a");
    ASSERT_EQ(vm.Pop().asInt(), 55);  // 10th Fibonacci number
}

TEST(operators_countdown_with_decrement)
{
    std::string code = R"(
        var n = 10;
        var sum = 0;
        
        while (n > 0) {
            sum += n--;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("n");
   //  printf("DEBUG: n = %d\n", vm.Pop().asInt());
    ASSERT_EQ(vm.Pop().asInt(), 0);
    
    vm.GetGlobal("sum");
     //  printf("DEBUG: sum = %d\n", vm.Pop().asInt());
   ASSERT_EQ(vm.Pop().asInt(), 55);  // 10+9+8+...+1
}

TEST(operators_power_of_two_with_multiply)
{
    std::string code = R"(
        var power = 1;
        for (var i = 0; i < 10; i++) {
            power *= 2;
        }
    )";
    Value result = executeProgram(code, "power");
    ASSERT_EQ(result.asInt(), 1024);  // 2^10
}

TEST(operators_complex_expression_in_loop)
{
    std::string code = R"(
        var result = 0;
        for (var i = 1; i <= 5; i++) {
            result += i * 2 + 3;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 45);  // (1*2+3) + (2*2+3) + (3*2+3) + (4*2+3) + (5*2+3)
}

TEST(operators_prefix_vs_postfix_comparison)
{
    std::string code = R"(
        var a = 5;
        var b = 5;
        var r1 = ++a;  // a=6, r1=6
        var r2 = b++;  // b=6, r2=5
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("a");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("b");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("r1");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("r2");
    ASSERT_EQ(vm.Pop().asInt(), 5);
}

TEST(operators_all_compound_assignments)
{
    std::string code = R"(
        var x = 10;
        x += 5;   // 15
        x -= 3;   // 12
        x *= 2;   // 24
        x /= 4;   // 6
        x %= 4;   // 2
    )";
    Value result = executeProgram(code, "x");
    ASSERT_EQ(result.asInt(), 2);
}

TEST(operators_increment_in_complex_condition)
{
    std::string code = R"(
        var count = 0;
        var i = 0;
        
        while (i++ < 5 && count < 10) {
            count++;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("i");
    ASSERT_EQ(vm.Pop().asInt(), 6);
    
    vm.GetGlobal("count");
    ASSERT_EQ(vm.Pop().asInt(), 5);
}

TEST(operators_decrement_to_zero)
{
    std::string code = R"(
        var n = 100;
        var iterations = 0;
        
        while (n > 0) {
            n -= 7;
            iterations++;
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("iterations");
    ASSERT_EQ(vm.Pop().asInt(), 15);  // Takes 15 iterations (100/7 rounded up)
}

TEST(operators_factorial_with_multiply_equals)
{
    std::string code = R"(
        var n = 6;
        var factorial = 1;
        
        while (n > 1) {
            factorial *= n;
            n--;
        }
    )";
    Value result = executeProgram(code, "factorial");
    ASSERT_EQ(result.asInt(), 720);  // 6!
}

TEST(do_while_executes_once)
{
    std::string code = R"(
        var x = 0;
        do {
            x++;
        } while (false);
    )";
    Value result = executeProgram(code, "x");
    ASSERT_EQ(result.asInt(), 1);  // Executa pelo menos 1 vez
}

TEST(do_while_loop)
{
    std::string code = R"(
        var i = 0;
        var sum = 0;
        do {
            sum += i;
            i++;
        } while (i < 5);
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 10);  // 0+1+2+3+4
}

TEST(loop_infinite_with_break)
{
    std::string code = R"(
        var count = 0;
        loop {
            count++;
            if (count >= 5) {
                break;
            }
        }
    )";
    Value result = executeProgram(code, "count");
    ASSERT_EQ(result.asInt(), 5);
}

TEST(loop_with_condition_break)
{
    std::string code = R"(
        var i = 0;
        var sum = 0;
        loop {
            if (i >= 10) {
                break;
            }
            sum += i;
            i++;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 45);  // 0+1+2+...+9
}

TEST(loop_with_continue)
{
    std::string code = R"(
        var i = 0;
        var sum = 0;
        loop {
            i++;
            if (i > 10) {
                break;
            }
            if (i % 2 == 0) {
                continue;
            }
            sum += i;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 25);  // 1+3+5+7+9
}

TEST(loop_nested)
{
    std::string code = R"(
        var outer = 0;
        var inner = 0;
        loop {
            outer++;
            if (outer > 3) {
                break;
            }
            loop {
                inner++;
                if (inner >= outer * 2) {
                    break;
                }
            }
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("outer");
    ASSERT_EQ(vm.Pop().asInt(), 4);
    
    vm.GetGlobal("inner");
    ASSERT_EQ(vm.Pop().asInt(), 6);  // 2 + 4 (soma dos limites)
}

TEST(loop_simulating_while)
{
    std::string code = R"(
        var n = 10;
        var factorial = 1;
        loop {
            if (n <= 1) {
                break;
            }
            factorial *= n;
            n--;
        }
    )";
    Value result = executeProgram(code, "factorial");
    ASSERT_EQ(result.asInt(), 3628800);  // 10!
}

TEST(loop_early_exit)
{
    std::string code = R"(
        var found = false;
        var i = 0;
        loop {
            i++;
            if (i == 42) {
                found = true;
                break;
            }
            if (i > 100) {
                break;
            }
        }
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("found");
    ASSERT_TRUE(vm.Pop().asBool());
    
    vm.GetGlobal("i");
    ASSERT_EQ(vm.Pop().asInt(), 42);
}


TEST(switch_simple)
{
    std::string code = R"(
        var x = 2;
        var result = 0;
        switch (x) {
            case 1:
                result = 10;
            case 2:
                result = 20;
            case 3:
                result = 30;
            default:
                result = 99;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 20);
}

TEST(switch_default)
{
    std::string code = R"(
        var x = 99;
        var result = 0;
        switch (x) {
            case 1:
                result = 10;
            case 2:
                result = 20;
            default:
                result = 999;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 999);
}

TEST(switch_first_case)
{
    std::string code = R"(
        var x = 1;
        var result = 0;
        switch (x) {
            case 1:
                result = 100;
            case 2:
                result = 200;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 100);
}

TEST(switch_with_expressions)
{
    std::string code = R"(
        var x = 5;
        var result = 0;
        switch (x * 2) {
            case 5:
                result = 1;
            case 10:
                result = 2;
            case 15:
                result = 3;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 2);  // 5*2 = 10
}

TEST(switch_no_match)
{
    std::string code = R"(
        var x = 99;
        var result = 5;
        switch (x) {
            case 1:
                result = 10;
            case 2:
                result = 20;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 5);  // Não mudou
}

TEST(switch_ultra_complex)
{
    std::string code = R"(
        def classify(n) {
            var result = "";
            switch (n % 3) {
                case 0:
                    result = "divisible by 3";
                case 1:
                    result = "remainder 1";
                case 2:
                    result = "remainder 2";
                default:
                    result = "impossible";
            }
            return result;
        }
        
        var r1 = classify(9);
        var r2 = classify(10);
        var r3 = classify(11);
    )";
    
    VM vm;
    vm.interpret(code);
    
    vm.GetGlobal("r1");
    ASSERT_EQ(std::string(vm.Pop().asString()), "divisible by 3");
    
    vm.GetGlobal("r2");
    ASSERT_EQ(std::string(vm.Pop().asString()), "remainder 1");
    
    vm.GetGlobal("r3");
    ASSERT_EQ(std::string(vm.Pop().asString()), "remainder 2");
}

TEST(switch_nested_in_loop)
{
    std::string code = R"(
        var total = 0;
        for (var i = 0; i < 10; i++) {
            switch (i % 3) {
                case 0:
                    total += 1;
                case 1:
                    total += 10;
                case 2:
                    total += 100;
            }
        }
    )";
    Value result = executeProgram(code, "total");
    // i=0: +1, i=1: +10, i=2: +100
    // i=3: +1, i=4: +10, i=5: +100
    // i=6: +1, i=7: +10, i=8: +100
    // i=9: +1
    // Total: 4*1 + 3*10 + 3*100 = 4 + 30 + 300 = 334
    ASSERT_EQ(result.asInt(), 334);
}

TEST(switch_with_loop_inside_case)
{
    std::string code = R"(
        var x = 2;
        var sum = 0;
        switch (x) {
            case 1:
                sum = 100;
            case 2:
                for (var i = 0; i < 5; i++) {
                    sum += i;
                }
            case 3:
                sum = 999;
        }
    )";
    Value result = executeProgram(code, "sum");
    ASSERT_EQ(result.asInt(), 10);  // 0+1+2+3+4
}

TEST(switch_expression_evaluation)
{
    std::string code = R"(
        var a = 5;
        var b = 3;
        var result = 0;
        switch (a + b) {
            case 7:
                result = 1;
            case 8:
                result = 2;
            case 9:
                result = 3;
            default:
                result = -1;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 2);  // 5+3=8
}

TEST(switch_all_strings)
{
    std::string code = R"(
        var cmd = "start";
        var result = 0;
        switch (cmd) {
            case "start":
                result = 1;
            case "stop":
                result = 2;
            case "pause":
                result = 3;
            default:
                result = -1;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 1);
}

TEST(switch_no_default_no_match)
{
    std::string code = R"(
        var x = 999;
        var result = 42;
        switch (x) {
            case 1:
                result = 1;
            case 2:
                result = 2;
        }
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 42);  // Não mudou
}

// TEST(debug_postfix_decrement_alone)
// {
//     std::string code = R"(
//         var n = 10;
//         var x = n--;
//         print(n);
//         print(x);
//     )";
    
//     VM vm;
//     vm.interpret(code);
    
//     vm.GetGlobal("n");
//     printf("n = %d (should be 9)\n", vm.Pop().asInt());
    
//     vm.GetGlobal("x");
//     printf("x = %d (should be 10)\n", vm.Pop().asInt());
// }

// TEST(debug_compound_add_with_value)
// {
//     std::string code = R"(
//         var sum = 0;
//         var val = 5;
//         sum += val;
//         print(sum);
//     )";
    
//     VM vm;
//     vm.interpret(code);
    
//     vm.GetGlobal("sum");
//     printf("sum = %d (should be 5)\n", vm.Pop().asInt());
// }

// TEST(debug_compound_add_with_postfix)
// {
//     std::string code = R"(
//         var sum = 0;
//         var n = 5;
//         sum += n--;
//         print(sum);
//         print(n);
//     )";
    
//     VM vm;
//     vm.interpret(code);
    
//     vm.GetGlobal("sum");
//     printf("sum = %d (should be 5)\n", vm.Pop().asInt());
    
//     vm.GetGlobal("n");
//     printf("n = %d (should be 4)\n", vm.Pop().asInt());
// }

// ============================================
// MAIN
// ============================================

int main()
{
    std::cout << "\n";

    // Tests run automatically via static constructors

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║            TEST SUMMARY                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "Total:  " << totalTests << "\n";
    std::cout << "✅ Pass: " << passedTests << "\n";
    if (failedTests != 0)
        std::cout << "❌ Fail: " << failedTests << "\n";

    if (failedTests == 0)
    {
        std::cout << "\n🎉 ALL  TESTS PASSED! 🎉\n";
        return 0;
    }
    else
    {
        std::cout << "\n💥 SOME TESTS FAILED 💥\n\n";
        return 1;
    }
}