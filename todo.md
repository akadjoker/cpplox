# PHASE 2: VARIABLES & SCOPE - IMPLEMENTATION GUIDE

## Overview
This guide provides step-by-step instructions to implement variables in your compiler.
Each section includes theory, implementation details, and test cases.

## Table of Contents
1. Global Variables
2. Local Variables & Scope
3. Variable Assignment
4. Testing Strategy

═══════════════════════════════════════════════════════════════════
## 1. GLOBAL VARIABLES
═══════════════════════════════════════════════════════════════════

### What You Need

**New Opcodes:**
- OP_DEFINE_GLOBAL - Store variable in globals table
- OP_GET_GLOBAL - Read variable from globals table
- OP_SET_GLOBAL - Update variable in globals table

**VM State:**
- `std::unordered_map<const char*, Value> globals_;` (already exists!)

### Implementation Steps

#### Step 1: Update varDeclaration() in compiler.cpp

```cpp
void Compiler::varDeclaration() {
    // 1. Consume variable name
    consume(TOKEN_IDENTIFIER, "Expect variable name");
    Token nameToken = previous;
    
    // 2. Create constant for name
    uint8_t global = identifierConstant(nameToken);
    
    // 3. Parse initializer (optional)
    if (match(TOKEN_EQUAL)) {
        expression();  // Compile the value
    } else {
        emitByte(OP_NIL);  // Default to nil
    }
    
    // 4. Expect semicolon
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    
    // 5. Emit define opcode
    defineVariable(global);
}
```

#### Step 2: Implement identifierConstant()

```cpp
uint8_t Compiler::identifierConstant(Token& name) {
    // Intern the string (already done by StringPool)
    const char* interned = StringPool::instance().intern(name.lexeme);
    
    // Add to constant pool
    return makeConstant(Value::makeString(interned));
}
```

#### Step 3: Implement defineVariable()

```cpp
void Compiler::defineVariable(uint8_t global) {
    if (scopeDepth > 0) {
        // Local variable - mark initialized (Phase 2.2)
        markInitialized();
        return;
    }
    
    // Global variable
    emitBytes(OP_DEFINE_GLOBAL, global);
}
```

#### Step 4: Update variable() for reading

```cpp
void Compiler::variable(bool canAssign) {
    namedVariable(previous, canAssign);
}

void Compiler::namedVariable(Token& name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(name);
    
    if (arg != -1) {
        // Local variable (Phase 2.2)
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        // Global variable
        arg = identifierConstant(name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}
```

#### Step 5: VM Implementation (already done!)

Your VM already has OP_GET_GLOBAL and OP_SET_GLOBAL implemented.
Just verify OP_DEFINE_GLOBAL exists.

### Testing Global Variables

```cpp
TEST(global_variable_declaration) {
    executeProgram(R"(
        var x = 42;
        print(x);
    )");
    // Expected output: 42
}

TEST(global_variable_assignment) {
    executeProgram(R"(
        var x = 10;
        x = 20;
        print(x);
    )");
    // Expected output: 20
}

TEST(multiple_globals) {
    executeProgram(R"(
        var a = 1;
        var b = 2;
        var c = a + b;
        print(c);
    )");
    // Expected output: 3
}
```

═══════════════════════════════════════════════════════════════════
## 2. LOCAL VARIABLES & SCOPE
═══════════════════════════════════════════════════════════════════

### What You Need

**New Opcodes:**
- OP_GET_LOCAL - Read from stack slot
- OP_SET_LOCAL - Write to stack slot

**Compiler State:**
- `int scopeDepth;` (already exists!)
- `std::vector<Local> locals;` (already exists!)

**Local struct:**
```cpp
struct Local {
    std::string name;
    int depth;
    Local(const std::string& n, int d) : name(n), depth(d) {}
};
```

### Implementation Steps

#### Step 1: Scope Management

```cpp
void Compiler::beginScope() {
    scopeDepth++;
}

void Compiler::endScope() {
    scopeDepth--;
    
    // Pop all locals from this scope
    while (!locals.empty() && 
           locals.back().depth > scopeDepth) {
        emitByte(OP_POP);
        locals.pop_back();
    }
}
```

#### Step 2: Block Statement

```cpp
void Compiler::block() {
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    
    consume(TOKEN_RBRACE, "Expect '}' after block");
}
```

Update statement() to handle blocks:
```cpp
void Compiler::statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_LBRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}
```

#### Step 3: Local Variable Declaration

```cpp
void Compiler::declareVariable() {
    if (scopeDepth == 0) return;  // Global
    
    Token& name = previous;
    
    // Check for duplicate in current scope
    for (int i = locals.size() - 1; i >= 0; i--) {
        Local& local = locals[i];
        if (local.depth != -1 && local.depth < scopeDepth) {
            break;  // Left current scope
        }
        
        if (name.lexeme == local.name) {
            error("Variable with this name already declared in this scope");
        }
    }
    
    addLocal(name);
}

void Compiler::addLocal(Token& name) {
    if (locals.size() >= UINT8_MAX) {
        error("Too many local variables in function");
        return;
    }
    
    locals.emplace_back(name.lexeme, -1);  // -1 = uninitialized
}

void Compiler::markInitialized() {
    if (scopeDepth == 0) return;
    locals.back().depth = scopeDepth;
}
```

#### Step 4: Resolve Local Variables

```cpp
int Compiler::resolveLocal(Token& name) {
    // Search backwards (innermost scope first)
    for (int i = locals.size() - 1; i >= 0; i--) {
        Local& local = locals[i];
        if (name.lexeme == local.name) {
            if (local.depth == -1) {
                error("Can't read local variable in its own initializer");
            }
            return i;
        }
    }
    
    return -1;  // Not found
}
```

#### Step 5: Update varDeclaration()

```cpp
void Compiler::varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect variable name");
    Token nameToken = previous;
    
    uint8_t global = identifierConstant(nameToken);
    
    // Declare BEFORE parsing initializer (prevents self-reference)
    if (scopeDepth > 0) {
        declareVariable();
    }
    
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    
    defineVariable(global);
}
```

### Testing Local Variables

```cpp
TEST(local_variables) {
    executeProgram(R"(
        {
            var x = 10;
            print(x);
        }
    )");
    // Expected: 10
}

TEST(nested_scopes) {
    executeProgram(R"(
        var x = 1;
        {
            var x = 2;
            print(x);
        }
        print(x);
    )");
    // Expected: 2, 1
}

TEST(scope_cleanup) {
    executeProgram(R"(
        {
            var a = 1;
            var b = 2;
            var c = 3;
        }
        // All should be popped here
    )");
}
```

═══════════════════════════════════════════════════════════════════
## 3. VARIABLE ASSIGNMENT
═══════════════════════════════════════════════════════════════════

### Already Implemented!

The namedVariable() function (from Section 1.4) already handles assignment:

```cpp
if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
} else {
    emitBytes(getOp, (uint8_t)arg);
}
```

### Testing Assignment

```cpp
TEST(variable_assignment) {
    executeProgram(R"(
        var x = 10;
        x = 20;
        print(x);
    )");
    // Expected: 20
}

TEST(assignment_in_expression) {
    executeProgram(R"(
        var x = 10;
        print(x = 20);
        print(x);
    )");
    // Expected: 20, 20
}

TEST(chained_assignment) {
    executeProgram(R"(
        var a = 1;
        var b = 2;
        a = b = 5;
        print(a);
        print(b);
    )");
    // Expected: 5, 5
}
```

═══════════════════════════════════════════════════════════════════
## 4. COMMON PITFALLS & DEBUGGING
═══════════════════════════════════════════════════════════════════

### Pitfall 1: Forgetting to Pop Locals
**Symptom:** Stack overflow or wrong values
**Fix:** Always call endScope() when leaving a block

### Pitfall 2: Wrong Scope Depth
**Symptom:** Variables not found or wrong variable used
**Fix:** Ensure beginScope()/endScope() are balanced

### Pitfall 3: Self-Referencing Initializer
```cpp
var x = x + 1;  // ERROR!
```
**Fix:** Already handled in resolveLocal() with depth == -1 check

### Pitfall 4: Shadowing Issues
```cpp
var x = 1;
{
    var x = x + 1;  // Should use outer x
}
```
**Fix:** declareVariable() is called BEFORE parsing initializer

### Debug Tips

1. **Print locals vector:**
```cpp
void Compiler::debugLocals() {
    fprintf(stderr, "=== LOCALS ===\n");
    for (size_t i = 0; i < locals.size(); i++) {
        fprintf(stderr, "[%zu] %s (depth: %d)\n", 
                i, locals[i].name.c_str(), locals[i].depth);
    }
}
```

2. **Disassemble chunks:** Use your existing disassembler!

3. **Step through VM:** Add breakpoints in executeInstruction()

═══════════════════════════════════════════════════════════════════
## 5. COMPLETE TEST SUITE
═══════════════════════════════════════════════════════════════════

```cpp
// Save this as test_variables.cpp

TEST(global_simple) {
    executeProgram("var x = 42; print(x);");
    // Output: 42
}

TEST(global_update) {
    executeProgram(R"(
        var x = 10;
        x = x + 5;
        print(x);
    )");
    // Output: 15
}

TEST(local_simple) {
    executeProgram(R"(
        {
            var x = 20;
            print(x);
        }
    )");
    // Output: 20
}

TEST(shadowing) {
    executeProgram(R"(
        var x = "global";
        {
            var x = "local";
            print(x);
        }
        print(x);
    )");
    // Output: local, global
}

TEST(multiple_scopes) {
    executeProgram(R"(
        var x = 1;
        {
            var y = 2;
            {
                var z = 3;
                print(x + y + z);
            }
            print(x + y);
        }
        print(x);
    )");
    // Output: 6, 3, 1
}

TEST(use_before_declare_error) {
    // Should fail compilation
    Compiler comp;
    Function* fn = comp.compile("print(x);");
    ASSERT_EQ(fn, nullptr);
}
```

═══════════════════════════════════════════════════════════════════
## 6. IMPLEMENTATION CHECKLIST
═══════════════════════════════════════════════════════════════════

### Before You Start
- [ ] Verify OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL exist
- [ ] Verify OP_GET_LOCAL, OP_SET_LOCAL exist
- [ ] Verify VM has globals_ map

### Implementation Order
- [ ] 1. identifierConstant()
- [ ] 2. defineVariable()
- [ ] 3. varDeclaration() (globals only first)
- [ ] 4. namedVariable()
- [ ] 5. variable()
- [ ] 6. Test globals
- [ ] 7. beginScope() / endScope()
- [ ] 8. block()
- [ ] 9. declareVariable()
- [ ] 10. addLocal()
- [ ] 11. resolveLocal()
- [ ] 12. markInitialized()
- [ ] 13. Update varDeclaration() for locals
- [ ] 14. Test locals
- [ ] 15. Test shadowing
- [ ] 16. Run full test suite

═══════════════════════════════════════════════════════════════════
## 7. NEXT STEPS (PHASE 3)
═══════════════════════════════════════════════════════════════════

After variables work, you'll implement:

1. **if/else statements**
   - Uses OP_JUMP_IF_FALSE and OP_JUMP
   - Need to patch jump addresses

2. **Logical operators (&&, ||)**
   - Short-circuit evaluation
   - Uses same jump opcodes

3. **while loops**
   - OP_LOOP to jump backwards
   - OP_JUMP_IF_FALSE for condition

