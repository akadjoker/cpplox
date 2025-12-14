# 🧠 Bytecode-Based Interpreted Language (WIP)

A **bytecode-based interpreted programming language**, built from scratch in C++, featuring its own compiler, virtual machine, and a Lua-style C/C++ integration API.

This project was created as an experimental but usable language, focused on learning, performance, and understanding how programming languages work internally.

---

## ✨ Motivation

After completing **minishell at 42 in 2023**, I became increasingly curious about how shells, compilers, and programming languages work under the hood.

Since then, I’ve studied compiler design (inspired by *Crafting Interpreters*) and built several language prototypes.
This project was a personal challenge:

> *Could I build a small but genuinely usable language in a short amount of time, using solid fundamentals and modern tools (including AI)?*

The answer turned out to be **yes**.

---

## 🏗️ Architecture Overview

The language follows a classic multi-stage architecture:

```
Source Code
    ↓
Lexer / Parser
    ↓
Compiler
    ↓
Bytecode
    ↓
Virtual Machine (VM)
```

* **Compiler**: Translates source code into compact bytecode
* **VM**: Executes bytecode using a stack-based execution model
* **Runtime**: Manages values, control flow, and native integration

---

## 🚀 Language Features

### 🔹 Core Language

* Dynamic typing
* Expressions with correct operator precedence and associativity
* Integers, floats, booleans, and strings
* String concatenation and comparison

### 🔹 Control Flow

* `if / else if / else`
* `switch / case`
* `for` loops
* `while` loops
* `do while` loops

### 🔹 Operators

* Infix operators: `+ - * / == != < <= > >=`
* Prefix operators: `++i`, `--i`, `!expr`
* Postfix operators: `i++`, `i--`
* Compound assignment: `i += 1`, `i -= 1`, etc.

### 🔹 Error Handling

* Parser error reporting (syntax errors)
* Runtime error handling
* Clear failure states during interpretation

---

## ⚙️ Virtual Machine

* Stack-based execution model
* Compact bytecode format
* Optimized symbol table (array + hash hybrid)
* Reasonable performance for a fully interpreted language

---

## 🔌 C++ ↔ VM Integration (Lua-style API)

One of the key features of this project is its **Lua-inspired stack-based C/C++ API**.

### What this allows:

* The host application (C++) can:

  * Push values onto the VM stack
  * Read and modify VM state
  * Call language functions
* The VM can:

  * Call native C++ functions
  * Exchange data bidirectionally with the host

This creates **true two-way communication between C++ and the VM**, making the language easy to embed and extend.

---

## 🧪 Testing

The project includes a comprehensive test suite that validates:

* Operator precedence and associativity
* Nested and complex expressions
* Mixed integer and floating-point arithmetic
* Comparison and logical operators
* Stress cases (deep nesting, long expression chains)
* Real-world calculations

Example test categories:

* Arithmetic correctness
* Boolean logic
* String operations
* Mathematical identities
* Edge cases and stress tests

All tests must pass for the interpreter to be considered stable.

  
---

## 🔮 Future Work

* Garbage collection
* User-defined functions
* Closures
* Modules / imports
* Better tooling and debugging support
* Bytecode optimizations

---

## 🧩 Inspiration & References

* *Crafting Interpreters* — Robert Nystrom
* Lua VM and C API design
* Classical compiler and VM architecture

---

## 📣 Final Notes

This project is not meant to compete with production languages.
It exists to **learn, experiment, and understand**.

If this repository inspires you to:

* study compilers,
* write a VM,
* or build your own language,

then it has already succeeded.

 