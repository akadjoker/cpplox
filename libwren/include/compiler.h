#pragma once

#include "lexer.h"
#include "chunk.h"
#include "value.h"
#include <string>
#include <vector>
#include <cstring>

class Compiler;
class VM;

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

#define MAX_IDENTIFIER_LENGTH 32
#define MAX_LOCALS 256

struct Local
{
    char name[MAX_IDENTIFIER_LENGTH];
    uint8_t length;
    int depth;

    Local() : length(0), depth(-1)
    {
        name[0] = '\0';
    }

    bool equals(const std::string &str) const
    {

        if (length != str.length())
        {
            return false;
        }

        return std::memcmp(name, str.c_str(), length) == 0;
    }

    bool equals(const char *str, size_t len) const
    {
        if (length != len)
        {
            return false;
        }
        return std::memcmp(name, str, length) == 0;
    }
};

#define MAX_LOOP_DEPTH 32
#define MAX_BREAKS_PER_LOOP 256

struct LoopContext
{
    int loopStart;
    int breakJumps[MAX_BREAKS_PER_LOOP];
    int breakCount;
    int scopeDepth;

    LoopContext() : loopStart(0), breakCount(0), scopeDepth(0) {}

    bool addBreak(int jump)
    {
        if (breakCount >= MAX_BREAKS_PER_LOOP)
        {

            return false;
        }
        breakJumps[breakCount++] = jump;
        return true;
    }
};

#define MAX_LOCALS 256
class Compiler
{
public:
    Compiler(VM *vm);
    ~Compiler();

    Function *compile(const std::string &source, VM *vm);
    Function *compileExpression(const std::string &source, VM *vm);

    void clear();

private:
    VM *vm_;
    Lexer *lexer;
    Token current;
    Token previous;

    Function *function;
    Chunk *currentChunk;

    bool hadError;
    bool panicMode;

    int scopeDepth;
    Local locals_[MAX_LOCALS];
    int localCount_;
  
    LoopContext loopContexts_[MAX_LOOP_DEPTH];
    int loopDepth_;
    bool isProcess_;
    // Token management
    void advance();
    bool check(TokenType type);
    bool match(TokenType type);
    void consume(TokenType type, const char *message);

    void beginLoop(int loopStart);
    void endLoop();
    void emitBreak();
    void emitContinue();

    void breakStatement();
    void continueStatement();

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
    void funDeclaration(bool isProcess=false);
    void expressionStatement();
    void printStatement();
    void ifStatement();
    void whileStatement();
    void doWhileStatement();
    void loopStatement();
    void switchStatement();
    void forStatement();
    void returnStatement();
    void block();

    void prefixIncrement(bool canAssign);
    void prefixDecrement(bool canAssign);

    // Variables
    uint8_t identifierConstant(Token &name);
    void namedVariable(Token &name, bool canAssign);
    void defineVariable(uint8_t global);
    void declareVariable();
    void addLocal(Token &name);
    int resolveLocal(Token &name);
    void markInitialized();

    uint8_t argumentList();

    void compileFunction(const std::string &name, bool isProcess=false);

    bool isProcessFunction(const char* name) const ;

    // Scope
    void beginScope();
    void endScope();

     bool inProcessFunction() const ;

    void initRules();

    void frameStatement();
    void exitStatement();

    static ParseRule rules[TOKEN_COUNT];
};