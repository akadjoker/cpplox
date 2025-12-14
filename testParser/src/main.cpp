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
    return ret;
}

// ============================================
// ADVANCED ARITHMETIC TESTS
// ============================================

TEST(operator_precedence_comprehensive)
{
    // Test: 2 + 3 * 4 = 14 (not 20)
    Value v = executeExpression("2 + 3 * 4");
    ASSERT_EQ(v.asInt(), 14);

    // Test: 10 - 6 / 2 = 7 (not 2)
    v = executeExpression("10 - 6 / 2");
    ASSERT_EQ(v.asInt(), 7);

    // Test: 15 / 3 + 2 * 4 = 13
    v = executeExpression("15 / 3 + 2 * 4");
    ASSERT_EQ(v.asInt(), 13);

    // Test: 20 - 10 / 5 + 3 = 21
    v = executeExpression("20 - 10 / 5 + 3");
    ASSERT_EQ(v.asInt(), 21);
}

TEST(parentheses_override_precedence)
{
    // Test: (2 + 3) * 4 = 20
    Value v = executeExpression("(2 + 3) * 4");
    ASSERT_EQ(v.asInt(), 20);

    // Test: (10 - 6) / 2 = 2
    v = executeExpression("(10 - 6) / 2");
    ASSERT_EQ(v.asInt(), 2);

    // Test: 2 * (3 + 4) - 5 = 9
    v = executeExpression("2 * (3 + 4) - 5");
    ASSERT_EQ(v.asInt(), 9);

    // Test: (20 - 10) / (5 + 3) = 1
    v = executeExpression("(20 - 10) / (5 + 3)");
    ASSERT_EQ(v.asInt(), 1);
}

TEST(left_associativity_subtraction)
{
    // Test: 10 - 5 - 2 = 3 (not 7)
    Value v = executeExpression("10 - 5 - 2");
    ASSERT_EQ(v.asInt(), 3);

    // Test: 100 - 50 - 25 - 10 = 15
    v = executeExpression("100 - 50 - 25 - 10");
    ASSERT_EQ(v.asInt(), 15);

    // Test: 20 - 3 - 2 - 1 = 14
    v = executeExpression("20 - 3 - 2 - 1");
    ASSERT_EQ(v.asInt(), 14);
}

TEST(left_associativity_division)
{
    // Test: 100 / 10 / 2 = 5 (not 20)
    Value v = executeExpression("100 / 10 / 2");
    ASSERT_EQ(v.asInt(), 5);

    // Test: 1000 / 10 / 5 / 2 = 10
    v = executeExpression("1000 / 10 / 5 / 2");
    ASSERT_EQ(v.asInt(), 10);
}

TEST(complex_nested_expressions)
{
    // Test: ((2 + 3) * 4 - 5) / 3 = 5
    Value v = executeExpression("((2 + 3) * 4 - 5) / 3");
    ASSERT_EQ(v.asInt(), 5);

    // Test: 2 * (3 + 4 * (5 - 2)) = 30
    v = executeExpression("2 * (3 + 4 * (5 - 2))");
    ASSERT_EQ(v.asInt(), 30);

    // Test: (10 + 20) * (30 - 20) / (2 + 3) = 60
    v = executeExpression("(10 + 20) * (30 - 20) / (2 + 3)");
    ASSERT_EQ(v.asInt(), 60);
}

TEST(mixed_int_float_operations)
{
    // Test: 5 + 2.5 = 7.5
    Value v = executeExpression("5 + 2.5");
    ASSERT_DOUBLE_EQ(v.asDouble(), 7.5);

    // Test: 10.0 - 3 = 7.0
    v = executeExpression("10.0 - 3");
    ASSERT_DOUBLE_EQ(v.asDouble(), 7.0);

    // Test: 3 * 2.5 = 7.5
    v = executeExpression("3 * 2.5");
    ASSERT_DOUBLE_EQ(v.asDouble(), 7.5);

    // Test: 15.0 / 4 = 3.75
    v = executeExpression("15.0 / 4");
    ASSERT_DOUBLE_EQ(v.asDouble(), 3.75);
}

TEST(negative_number_handling)
{
    // Test: -5 + 10 = 5
    Value v = executeExpression("-5 + 10");
    ASSERT_EQ(v.asInt(), 5);

    // Test: 10 + -5 = 5
    v = executeExpression("10 + -5");
    ASSERT_EQ(v.asInt(), 5);

    // Test: -10 - -5 = -5
    v = executeExpression("-10 - -5");
    ASSERT_EQ(v.asInt(), -5);

    // Test: -3 * -4 = 12
    v = executeExpression("-3 * -4");
    ASSERT_EQ(v.asInt(), 12);

    // Test: -20 / -4 = 5
    v = executeExpression("-20 / -4");
    ASSERT_EQ(v.asInt(), 5);
}

TEST(double_negation)
{
    // Test: --5 = 5
    Value v = executeExpression("--5");
    ASSERT_EQ(v.asInt(), 5);

    // Test: ---10 = -10
    v = executeExpression("---10");
    ASSERT_EQ(v.asInt(), -10);

    // Test: ----100 = 100
    v = executeExpression("----100");
    ASSERT_EQ(v.asInt(), 100);
}

// ============================================
// COMPARISON OPERATOR TESTS
// ============================================

TEST(equality_comprehensive)
{
    // Integers
    Value v = executeExpression("5 == 5");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("5 == 6");
    ASSERT_EQ(v.asBool(), false);

    // Floats
    v = executeExpression("3.14 == 3.14");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("3.14 == 3.15");
    ASSERT_EQ(v.asBool(), false);

    // Mixed types
    v = executeExpression("5 == 5.0");
    ASSERT_EQ(v.asBool(), false); // Different types

    // Booleans
    v = executeExpression("true == true");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("true == false");
    ASSERT_EQ(v.asBool(), false);

    // Strings
    v = executeExpression("\"hello\" == \"hello\"");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("\"hello\" == \"world\"");
    ASSERT_EQ(v.asBool(), false);
}

TEST(inequality_comprehensive)
{
    Value v = executeExpression("5 != 6");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("5 != 5");
    ASSERT_EQ(v.asBool(), false);

    v = executeExpression("\"hello\" != \"world\"");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("true != false");
    ASSERT_EQ(v.asBool(), true);
}

TEST(comparison_chains)
{
    // Test: 5 < 10 is true
    Value v = executeExpression("5 < 10");
    ASSERT_EQ(v.asBool(), true);

    // Test: 10 < 5 is false
    v = executeExpression("10 < 5");
    ASSERT_EQ(v.asBool(), false);

    // Test: 5 <= 5 is true
    v = executeExpression("5 <= 5");
    ASSERT_EQ(v.asBool(), true);

    // Test: 5 <= 4 is false
    v = executeExpression("5 <= 4");
    ASSERT_EQ(v.asBool(), false);

    // Test: 10 > 5 is true
    v = executeExpression("10 > 5");
    ASSERT_EQ(v.asBool(), true);

    // Test: 5 >= 5 is true
    v = executeExpression("5 >= 5");
    ASSERT_EQ(v.asBool(), true);

    // Test: 4 >= 5 is false
    v = executeExpression("4 >= 5");
    ASSERT_EQ(v.asBool(), false);
}

TEST(comparison_with_expressions)
{
    // Test: (5 + 3) > 7 = true
    Value v = executeExpression("(5 + 3) > 7");
    ASSERT_EQ(v.asBool(), true);

    // Test: (10 - 3) < 5 = false
    v = executeExpression("(10 - 3) < 5");
    ASSERT_EQ(v.asBool(), false);

    // Test: (2 * 5) == 10 = true
    v = executeExpression("(2 * 5) == 10");
    ASSERT_EQ(v.asBool(), true);

    // Test: (20 / 4) >= 5 = true
    v = executeExpression("(20 / 4) >= 5");
    ASSERT_EQ(v.asBool(), true);
}

// ============================================
// LOGICAL OPERATOR TESTS (using NOT)
// ============================================

TEST(not_operator_comprehensive)
{
    // Test: !true = false
    Value v = executeExpression("!true");
    ASSERT_EQ(v.asBool(), false);

    // // Test: !false = true
    v = executeExpression("!false");
    ASSERT_EQ(v.asBool(), true);

    // // Test: !!true = true
    v = executeExpression("!!true");
    ASSERT_EQ(v.asBool(), true);

    // // Test: !!!false = false
    v = executeExpression("!!!false");
    ASSERT_EQ(v.asBool(), true);
}

TEST(not_with_comparisons)
{
    // Test: !(5 > 10) = true
    Value v = executeExpression("!(5 > 10)");
    ASSERT_EQ(v.asBool(), true);

    // Test: !(10 == 10) = false
    v = executeExpression("!(10 == 10)");
    ASSERT_EQ(v.asBool(), false);

    // Test: !(5 < 3) = true
    v = executeExpression("!(5 < 3)");
    ASSERT_EQ(v.asBool(), true);
}

// ============================================
// EDGE CASES & STRESS TESTS
// ============================================

TEST(zero_operations)
{
    // Test: 0 + 0 = 0
    Value v = executeExpression("0 + 0");
    ASSERT_EQ(v.asInt(), 0);

    // Test: 5 - 5 = 0
    v = executeExpression("5 - 5");
    ASSERT_EQ(v.asInt(), 0);

    // Test: 0 * 100 = 0
    v = executeExpression("0 * 100");
    ASSERT_EQ(v.asInt(), 0);

    // Test: 0 / 1 = 0
    v = executeExpression("0 / 1");
    ASSERT_EQ(v.asInt(), 0);
}

TEST(large_numbers)
{
    // Test: 999999 + 1 = 1000000
    Value v = executeExpression("999999 + 1");
    ASSERT_EQ(v.asInt(), 1000000);

    // Test: 1000000 - 1 = 999999
    v = executeExpression("1000000 - 1");
    ASSERT_EQ(v.asInt(), 999999);
}

TEST(very_deep_nesting)
{
    // Test: (((((5))))) = 5
    Value v = executeExpression("(((((5)))))");
    ASSERT_EQ(v.asInt(), 5);

    // Test deeply nested arithmetic
    v = executeExpression("((((1 + 2) + 3) + 4) + 5)");
    ASSERT_EQ(v.asInt(), 15);
}

TEST(long_chain_operations)
{
    // Test: 1+1+1+1+1+1+1+1+1+1 = 10
    Value v = executeExpression("1+1+1+1+1+1+1+1+1+1");
    ASSERT_EQ(v.asInt(), 10);

    // Test: 2*2*2*2*2 = 32
    v = executeExpression("2*2*2*2*2");
    ASSERT_EQ(v.asInt(), 32);
}

TEST(precedence_with_all_operators)
{
    // Test: 2 + 3 * 4 - 5 / 1 = 9
    // = 2 + 12 - 5 = 9
    Value v = executeExpression("2 + 3 * 4 - 5 / 1");
    ASSERT_EQ(v.asInt(), 9);

    // Test: 10 / 2 + 3 * 4 - 5 = 12
    // = 5 + 12 - 5 = 12
    v = executeExpression("10 / 2 + 3 * 4 - 5");
    ASSERT_EQ(v.asInt(), 12);
}

TEST(comparison_precedence)
{
    // Comparison has lower precedence than arithmetic
    // Test: 5 + 3 > 7 = true (8 > 7)
    Value v = executeExpression("5 + 3 > 7");
    ASSERT_EQ(v.asBool(), true);

    // Test: 10 - 5 < 3 = false (5 < 3)
    v = executeExpression("10 - 5 < 3");
    ASSERT_EQ(v.asBool(), false);

    // Test: 2 * 3 == 6 = true
    v = executeExpression("2 * 3 == 6");
    ASSERT_EQ(v.asBool(), true);
}

TEST(string_operations)
{
    // String concatenation
    Value v = executeExpression("\"Hello\" + \" \" + \"World\"");
    ASSERT_EQ(std::string(v.asString()), "Hello World");

    // Empty string
    v = executeExpression("\"\" + \"test\"");
    ASSERT_EQ(std::string(v.asString()), "test");

    // String equality
    v = executeExpression("\"abc\" == \"abc\"");
    ASSERT_EQ(v.asBool(), true);

    v = executeExpression("\"abc\" != \"xyz\"");
    ASSERT_EQ(v.asBool(), true);
}

TEST(mathematical_identities)
{
    // Test: x - x = 0
    Value v = executeExpression("42 - 42");
    ASSERT_EQ(v.asInt(), 0);

    // Test: x + 0 = x
    v = executeExpression("42 + 0");
    ASSERT_EQ(v.asInt(), 42);

    // Test: x * 1 = x
    v = executeExpression("42 * 1");
    ASSERT_EQ(v.asInt(), 42);

    // Test: x / 1 = x
    v = executeExpression("42 / 1");
    ASSERT_EQ(v.asInt(), 42);

    // Test: x * 0 = 0
    v = executeExpression("42 * 0");
    ASSERT_EQ(v.asInt(), 0);
}

TEST(distributive_property)
{
    // Test: 2 * (3 + 4) == 2 * 3 + 2 * 4
    Value v1 = executeExpression("2 * (3 + 4)");
    Value v2 = executeExpression("2 * 3 + 2 * 4");
    ASSERT_EQ(v1.asInt(), v2.asInt());
    ASSERT_EQ(v1.asInt(), 14);
}

TEST(commutative_property)
{
    // Test: a + b == b + a
    Value v1 = executeExpression("5 + 7");
    Value v2 = executeExpression("7 + 5");
    ASSERT_EQ(v1.asInt(), v2.asInt());

    // Test: a * b == b * a
    v1 = executeExpression("3 * 9");
    v2 = executeExpression("9 * 3");
    ASSERT_EQ(v1.asInt(), v2.asInt());
}

// ============================================
// REAL-WORLD CALCULATION TESTS
// ============================================

TEST(calculate_average)
{
    // Average of 10, 20, 30 = 20
    Value v = executeExpression("(10 + 20 + 30) / 3");
    ASSERT_EQ(v.asInt(), 20);
}

TEST(calculate_area)
{
    // Rectangle area: width * height
    Value v = executeExpression("15 * 8");
    ASSERT_EQ(v.asInt(), 120);

    // Circle area approximation: 3 * r * r (where r = 5)
    v = executeExpression("3 * 5 * 5");
    ASSERT_EQ(v.asInt(), 75);
}

TEST(calculate_percentage)
{
    // 25% of 200 = 50
    Value v = executeExpression("200 * 25 / 100");
    ASSERT_EQ(v.asInt(), 50);

    // 15% of 80 = 12
    v = executeExpression("80 * 15 / 100");
    ASSERT_EQ(v.asInt(), 12);
}

TEST(temperature_conversion)
{
    // Celsius to Fahrenheit: (C * 9/5) + 32
    // 0°C = 32°F
    Value v = executeExpression("(0 * 9 / 5) + 32");
    ASSERT_EQ(v.asInt(), 32);

    // 100°C = 212°F
    v = executeExpression("(100 * 9 / 5) + 32");
    ASSERT_EQ(v.asInt(), 212);
}


// ============================================
// MAIN
// ============================================

int main()
{
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║   ADVANCED MATHEMATICAL TEST SUITE     ║\n";
    std::cout << "║    (Rigorous Expression Validation)    ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";

    // Tests run automatically via static constructors

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║            TEST SUMMARY                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "Total:  " << totalTests << "\n";
    std::cout << "✅ Pass: " << passedTests << "\n";
    std::cout << "❌ Fail: " << failedTests << "\n";

    if (failedTests == 0)
    {
        std::cout << "\n🎉 ALL ADVANCED TESTS PASSED! 🎉\n";
        std::cout << "Your compiler correctly handles:\n";
        std::cout << "  ✓ Operator precedence\n";
        std::cout << "  ✓ Associativity rules\n";
        std::cout << "  ✓ Nested expressions\n";
        std::cout << "  ✓ Mixed types\n";
        std::cout << "  ✓ Comparison operators\n";
        std::cout << "  ✓ Mathematical identities\n";
        std::cout << "  ✓ Real-world calculations\n\n";
        return 0;
    }
    else
    {
        std::cout << "\n💥 SOME TESTS FAILED 💥\n\n";
        return 1;
    }
}