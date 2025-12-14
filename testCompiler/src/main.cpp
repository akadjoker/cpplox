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
        for (var i = 0; i < 3; i = i + 1) {
            var x = i * 10;
        }
        // i e x não existem aqui - não dá erro mas também não são acessíveis
        var result = 42;
    )";
    Value result = executeProgram(code, "result");
    ASSERT_EQ(result.asInt(), 42);
}

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