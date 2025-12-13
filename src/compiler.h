#pragma once

#include "lexer.h"
#include "chunk.h"
#include "value.h"
#include <string>
#include <vector>

class Compiler;

typedef void (Compiler::*ParseFn)(bool canAssign);

enum Precedence
{
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
};

struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
};

struct Local
{
    std::string name;
    int depth;

    Local(const std::string &n, int d) : name(n), depth(d) {}
};

class Compiler
{
public:
    Compiler();
    ~Compiler();

    Function *compile(const std::string &source);
    Function *compileExpression(const std::string &source);

private:
    Lexer *lexer;
    Token current;
    Token previous;

    Function *function;
    Chunk *currentChunk;

    bool hadError;
    bool panicMode;

    int scopeDepth;
    std::vector<Local> locals;

    // Token management
    void advance();
    bool check(TokenType type);
    bool match(TokenType type);
    void consume(TokenType type, const char *message);

    // Error handling
    void error(const char *message);
    void errorAt(Token &token, const char *message);
    void errorAtCurrent(const char *message);
    void synchronize();

    // Bytecode emission
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitReturn();
    void emitConstant(Value value);
    uint8_t makeConstant(Value value);

    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    // Pratt parser
    void expression();
    void parsePrecedence(Precedence precedence);
    ParseRule *getRule(TokenType type);

    // Parse functions (prefix)
    void number(bool canAssign);
    void string(bool canAssign);
    void literal(bool canAssign);
    void grouping(bool canAssign);
    void unary(bool canAssign);
    void variable(bool canAssign);

    // Parse functions (infix)
    void binary(bool canAssign);
    void and_(bool canAssign);
    void or_(bool canAssign);
    void call(bool canAssign);

    // Statements
    void declaration();
    void statement();
    void varDeclaration();
    void funDeclaration();
    void expressionStatement();
    void printStatement();
    void ifStatement();
    void whileStatement();
    void forStatement();
    void returnStatement();
    void block();

    // Variables
    uint8_t identifierConstant(Token &name);
    void namedVariable(Token &name, bool canAssign);
    void defineVariable(uint8_t global);
    void declareVariable();
    void addLocal(Token &name);
    int resolveLocal(Token &name);
    void markInitialized();

    // Scope
    void beginScope();
    void endScope();

    void initRules();

   static ParseRule rules[TOKEN_COUNT]; 
};