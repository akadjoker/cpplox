#include "compiler.h"
#include "vm.h"
#include "debug.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <type_traits>

// ============================================
// TEST FRAMEWORK
// ============================================

int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"; \
            std::cout << "🧪 TEST: " << #name << "\n"; \
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"; \
            totalTests++; \
            try { \
                test_##name(); \
                passedTests++; \
                std::cout << "✅ PASSED\n"; \
            } catch (const std::exception& e) { \
                failedTests++; \
                std::cout << "❌ FAILED: " << e.what() << "\n"; \
            } \
        } \
    } testRegistrar_##name; \
    void test_##name()

// Helper for converting values to string
template<typename T>
std::string valueToString(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) {
        return "\"" + val + "\"";
    } else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
        return std::string("\"") + val + "\"";
    } else if constexpr (std::is_same_v<T, bool>) {
        return val ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(val);
    } else {
        return "<unknown>";
    }
}

#define ASSERT_EQ(a, b) \
    do { \
        auto val_a = (a); \
        auto val_b = (b); \
        if (val_a != val_b) { \
            throw std::runtime_error( \
                std::string("Assertion failed: ") + #a + " == " + #b + \
                "\n  Expected: " + valueToString(val_b) + \
                "\n  Got:      " + valueToString(val_a)); \
        } \
    } while(0)

#define ASSERT_DOUBLE_EQ(a, b) \
    do { \
        double val_a = (a); \
        double val_b = (b); \
        if (std::abs(val_a - val_b) > 0.0001) { \
            throw std::runtime_error( \
                std::string("Assertion failed: ") + #a + " ≈ " + #b + \
                "\n  Expected: " + std::to_string(val_b) + \
                "\n  Got:      " + std::to_string(val_a)); \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error( \
                std::string("Assertion failed: ") + #cond + " is true"); \
        } \
    } while(0)

#define ASSERT_COMPILE_ERROR(code) \
    do { \
        Compiler comp; \
        Function* fn = comp.compile(code); \
        if (fn != nullptr) { \
            delete fn; \
            throw std::runtime_error( \
                std::string("Expected compile error but compilation succeeded: ") + code); \
        } \
    } while(0)

// Helper: Execute expression and get result
Value executeExpression(const std::string& code) {
    Compiler compiler;
    Function* fn = compiler.compileExpression(code);
    
    if (!fn) {
        throw std::runtime_error("Compilation failed: " + code);
    }
    
    VM vm;
    InterpretResult result = vm.interpret(fn);
    
    if (result != InterpretResult::OK) {
        delete fn;
        throw std::runtime_error("Runtime error: " + code);
    }
    
    Value ret = vm.Pop();
    delete fn;
    return ret;
}

// Helper: Execute program
void executeProgram(const std::string& code) {
    Compiler compiler;
    Function* fn = compiler.compile(code);
    
    if (!fn) {
        throw std::runtime_error("Compilation failed: " + code);
    }
    
    VM vm;
    InterpretResult result = vm.interpret(fn);
    
    delete fn;
    
    if (result != InterpretResult::OK) {
        throw std::runtime_error("Runtime error: " + code);
    }
}

// ============================================
// PHASE 1: LITERALS & BASIC ARITHMETIC
// ============================================

TEST(integer_literals) {
    Value v = executeExpression("42");
    ASSERT_TRUE(v.isInt());
    ASSERT_EQ(v.asInt(), 42);
    
    v = executeExpression("0");
    ASSERT_EQ(v.asInt(), 0);
    
    v = executeExpression("999999");
    ASSERT_EQ(v.asInt(), 999999);
}

TEST(float_literals) {
    Value v = executeExpression("3.14");
    ASSERT_TRUE(v.isDouble());
    ASSERT_DOUBLE_EQ(v.asDouble(), 3.14);
    
    v = executeExpression("0.5");
    ASSERT_DOUBLE_EQ(v.asDouble(), 0.5);
}

TEST(boolean_literals) {
    Value v = executeExpression("true");
    ASSERT_TRUE(v.isBool());
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("false");
    ASSERT_EQ(v.asBool(), false);
}

TEST(nil_literal) {
    Value v = executeExpression("nil");
    ASSERT_TRUE(v.isNull());
}

TEST(string_literals) {
    Value v = executeExpression("\"hello\"");
    ASSERT_TRUE(v.isString());
    ASSERT_EQ(std::string(v.asString()), "hello");
    
    v = executeExpression("\"\"");
    ASSERT_EQ(std::string(v.asString()), "");
}

// ============================================
// ARITHMETIC OPERATORS
// ============================================

TEST(addition_int) {
    Value v = executeExpression("10 + 5");
    ASSERT_EQ(v.asInt(), 15);
    
    v = executeExpression("1 + 2 + 3");
    ASSERT_EQ(v.asInt(), 6);
}

TEST(addition_float) {
    Value v = executeExpression("3.5 + 2.5");
    ASSERT_DOUBLE_EQ(v.asDouble(), 6.0);
}

TEST(addition_mixed) {
    Value v = executeExpression("10 + 3.5");
    ASSERT_DOUBLE_EQ(v.asDouble(), 13.5);
}

TEST(string_concatenation) {
    Value v = executeExpression("\"hello\" + \" \" + \"world\"");
    ASSERT_EQ(std::string(v.asString()), "hello world");
}

TEST(subtraction) {
    Value v = executeExpression("10 - 5");
    ASSERT_EQ(v.asInt(), 5);
    
    v = executeExpression("20 - 5 - 3");
    ASSERT_EQ(v.asInt(), 12);
}

TEST(multiplication) {
    Value v = executeExpression("5 * 3");
    ASSERT_EQ(v.asInt(), 15);
    
    v = executeExpression("2 * 3 * 4");
    ASSERT_EQ(v.asInt(), 24);
}

TEST(division) {
    Value v = executeExpression("20 / 5");
    ASSERT_EQ(v.asInt(), 4);
    
    v = executeExpression("10.0 / 4.0");
    ASSERT_DOUBLE_EQ(v.asDouble(), 2.5);
}

// ============================================
// PRECEDENCE & ASSOCIATIVITY
// ============================================

TEST(precedence_mult_over_add) {
    Value v = executeExpression("10 + 5 * 2");
    ASSERT_EQ(v.asInt(), 20); // Not 30
}

TEST(precedence_div_over_sub) {
    Value v = executeExpression("20 - 10 / 2");
    ASSERT_EQ(v.asInt(), 15); // Not 5
}

TEST(associativity_left) {
    Value v = executeExpression("10 - 5 - 2");
    ASSERT_EQ(v.asInt(), 3); // (10 - 5) - 2, not 10 - (5 - 2) = 7
    
    v = executeExpression("20 / 4 / 2");
    ASSERT_EQ(v.asInt(), 2); // (20 / 4) / 2, not 20 / (4 / 2) = 10
}

TEST(grouping_overrides_precedence) {
    Value v = executeExpression("(10 + 5) * 2");
    ASSERT_EQ(v.asInt(), 30);
    
    v = executeExpression("2 * (3 + 4)");
    ASSERT_EQ(v.asInt(), 14);
}

TEST(nested_grouping) {
    Value v = executeExpression("((2 + 3) * (4 + 5))");
    ASSERT_EQ(v.asInt(), 45);
}

// ============================================
// UNARY OPERATORS
// ============================================

TEST(unary_negation) {
    Value v = executeExpression("-42");
    ASSERT_EQ(v.asInt(), -42);
    
    v = executeExpression("-(10 + 5)");
    ASSERT_EQ(v.asInt(), -15);
    
    v = executeExpression("--10");
    ASSERT_EQ(v.asInt(), 10); // Double negation
}

TEST(unary_not) {
    Value v = executeExpression("!true");
    ASSERT_EQ(v.asBool(), false);
    
    v = executeExpression("!false");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("!!true");
    ASSERT_EQ(v.asBool(), true);
}

// ============================================
// COMPARISON OPERATORS
// ============================================

TEST(equality) {
    Value v = executeExpression("10 == 10");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 == 5");
    ASSERT_EQ(v.asBool(), false);
    
    v = executeExpression("\"hello\" == \"hello\"");
    ASSERT_EQ(v.asBool(), true);
}

TEST(inequality) {
    Value v = executeExpression("10 != 5");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 != 10");
    ASSERT_EQ(v.asBool(), false);
}

TEST(less_than) {
    Value v = executeExpression("5 < 10");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 < 5");
    ASSERT_EQ(v.asBool(), false);
    
    v = executeExpression("10 < 10");
    ASSERT_EQ(v.asBool(), false);
}

TEST(less_equal) {
    Value v = executeExpression("5 <= 10");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 <= 10");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 <= 5");
    ASSERT_EQ(v.asBool(), false);
}

TEST(greater_than) {
    Value v = executeExpression("10 > 5");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("5 > 10");
    ASSERT_EQ(v.asBool(), false);
}

TEST(greater_equal) {
    Value v = executeExpression("10 >= 5");
    ASSERT_EQ(v.asBool(), true);
    
    v = executeExpression("10 >= 10");
    ASSERT_EQ(v.asBool(), true);
}

// ============================================
// COMPLEX EXPRESSIONS
// ============================================

TEST(complex_arithmetic) {
    Value v = executeExpression("(5 + 3) * 2 - 10 / 2");
    ASSERT_EQ(v.asInt(), 11); // 8 * 2 - 5 = 11
}

TEST(mixed_types) {
    Value v = executeExpression("10 + 3.5 * 2");
    ASSERT_DOUBLE_EQ(v.asDouble(), 17.0);
}

// ============================================
// ERROR CASES
// ============================================

TEST(unclosed_parenthesis) {
    ASSERT_COMPILE_ERROR("(10 + 5");
}

TEST(unexpected_token) {
    ASSERT_COMPILE_ERROR("10 + + 5");
}

 

TEST(invalid_operator_usage) {
    ASSERT_COMPILE_ERROR("* 10");
    ASSERT_COMPILE_ERROR("10 +");
}

// ============================================
// EDGE CASES
// ============================================

TEST(deeply_nested_expressions) {
    Value v = executeExpression("((((10))))");
    ASSERT_EQ(v.asInt(), 10);
    
    v = executeExpression("(((1 + 2) + 3) + 4)");
    ASSERT_EQ(v.asInt(), 10);
}

TEST(whitespace_handling) {
    Value v = executeExpression("  10  +  5  ");
    ASSERT_EQ(v.asInt(), 15);
    
    v = executeExpression("10+5");
    ASSERT_EQ(v.asInt(), 15);
}

TEST(multiline_expression) {
    Value v = executeExpression(R"(
        10
        + 5
        * 2
    )");
    ASSERT_EQ(v.asInt(), 20);
}

// ============================================
// STATEMENTS (Basic)
// ============================================

TEST(print_statement) {
    std::cout << "  Expected output: 42\n  Actual output:   ";
    executeProgram("print(42);");
}

TEST(expression_statement) {
    executeProgram("10 + 5;");
    executeProgram("\"hello\";");
}

TEST(multiple_statements) {
    std::cout << "  Expected output: 10 20 30\n  Actual output:   ";
    executeProgram(R"(
        print(10);
        print(20);
        print(30);
    )");
}



TEST(long_expression) {
    std::string expr = "1";
    for (int i = 0; i < 50; i++) {
        expr += " + 1";
    }
    Value v = executeExpression(expr);
    ASSERT_EQ(v.asInt(), 51);
}

TEST(deep_nesting) {
    std::string expr = "10";
    for (int i = 0; i < 20; i++) {
        expr = "(" + expr + " + 1)";
    }
    Value v = executeExpression(expr);
    ASSERT_EQ(v.asInt(), 30);
}

// ============================================
// MAIN
// ============================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║     COMPILER TEST SUITE - PHASE 1      ║\n";
    std::cout << "║   (Literals & Basic Expressions)       ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    
    // Tests run automatically via static constructors
    
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║            TEST SUMMARY                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    std::cout << "Total:  " << totalTests << "\n";
    std::cout << "✅ Pass: " << passedTests << "\n";
    std::cout << "❌ Fail: " << failedTests << "\n";
    
    if (failedTests == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED! 🎉\n\n";
        return 0;
    } else {
        std::cout << "\n💥 SOME TESTS FAILED 💥\n\n";
        return 1;
    }
}

 

// void quick_debug() {
//     printf("\n=== QUICK DEBUG ===\n");
    
//     // Test 1: String parsing
//     Lexer lex("\"hello\"");
//     Token t = lex.scanToken();
//     printf("String token: type=%d, lexeme='%s'\n", t.type, t.lexeme.c_str());
    
//     // Test 2: Equality
//     Compiler comp;
//     Function* fn = comp.compileExpression("10 == 10");
//     printf("Equality compile: %s\n", fn ? "SUCCESS" : "FAILED");
//     if (fn) delete fn;
    
//     // Test 3: OP_NOT exists?
//     printf("OP_NOT defined: %d\n", OP_NOT);
    
//     printf("===================\n\n");
// }

// int main() {
//     quick_debug();  // ← ADICIONA ISTO
    
//     // ... resto dos testes ...
// }