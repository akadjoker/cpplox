#include "compiler.h"
#include "stringpool.h"
#include <cstdio>
#include <cstdlib>

// ============================================
// PARSE RULE TABLE - DEFINIÇÃO
// ============================================
constexpr ParseRule makeRule(ParseFn prefix, ParseFn infix, Precedence prec)
{
    return {prefix, infix, prec};
}

ParseRule Compiler::rules[TOKEN_COUNT];

// ============================================
// CONSTRUCTOR
// ============================================

Compiler::Compiler()
    : lexer(nullptr), function(nullptr), currentChunk(nullptr),
      hadError(false), panicMode(false), scopeDepth(0)
{

    initRules();
}

Compiler::~Compiler()
{
    delete lexer;
}

// ============================================
// INICIALIZAÇÃO DA TABELA
// ============================================

void Compiler::initRules()
{
    // Inicializa todos com nullptr primeiro
    for (int i = 0; i < TOKEN_COUNT; i++)
    {
        rules[i] = {nullptr, nullptr, PREC_NONE};
    }

    // Agora define os que têm funções
    rules[TOKEN_LPAREN] = {&Compiler::grouping, &Compiler::call, PREC_CALL};
    rules[TOKEN_RPAREN] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_LBRACE] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_RBRACE] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_COMMA] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_SEMICOLON] = {nullptr, nullptr, PREC_NONE};

    // Arithmetic
    rules[TOKEN_PLUS] = {nullptr, &Compiler::binary, PREC_TERM};
    rules[TOKEN_MINUS] = {&Compiler::unary, &Compiler::binary, PREC_TERM};
    rules[TOKEN_STAR] = {nullptr, &Compiler::binary, PREC_FACTOR};
    rules[TOKEN_SLASH] = {nullptr, &Compiler::binary, PREC_FACTOR};
    rules[TOKEN_PERCENT] = {nullptr, &Compiler::binary, PREC_FACTOR};

    // Comparison
    rules[TOKEN_EQUAL] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_EQUAL_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
    rules[TOKEN_BANG_EQUAL] = {nullptr, &Compiler::binary, PREC_EQUALITY};
    rules[TOKEN_LESS] = {nullptr, &Compiler::binary, PREC_COMPARISON};
    rules[TOKEN_LESS_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};
    rules[TOKEN_GREATER] = {nullptr, &Compiler::binary, PREC_COMPARISON};
    rules[TOKEN_GREATER_EQUAL] = {nullptr, &Compiler::binary, PREC_COMPARISON};

    // Logical
    rules[TOKEN_AND_AND] = {nullptr, &Compiler::and_, PREC_AND};
    rules[TOKEN_OR_OR] = {nullptr, &Compiler::or_, PREC_OR};
    rules[TOKEN_BANG] = {&Compiler::unary, nullptr, PREC_NONE};

    // Literals
    rules[TOKEN_INT] = {&Compiler::number, nullptr, PREC_NONE};
    rules[TOKEN_FLOAT] = {&Compiler::number, nullptr, PREC_NONE};
    rules[TOKEN_STRING] = {&Compiler::string, nullptr, PREC_NONE};
    rules[TOKEN_IDENTIFIER] = {&Compiler::variable, nullptr, PREC_NONE};
    rules[TOKEN_TRUE] = {&Compiler::literal, nullptr, PREC_NONE};
    rules[TOKEN_FALSE] = {&Compiler::literal, nullptr, PREC_NONE};
    rules[TOKEN_NIL] = {&Compiler::literal, nullptr, PREC_NONE};

    // Keywords (all nullptr já setados no loop)
    rules[TOKEN_EOF] = {nullptr, nullptr, PREC_NONE};
    rules[TOKEN_ERROR] = {nullptr, nullptr, PREC_NONE};
}

// ============================================
// MAIN ENTRY POINT
// ============================================

// compiler.cpp
Function* Compiler::compile(const std::string& source) {
    lexer = new Lexer(source);
    
    function = new Function("__main__", 0);
    currentChunk = &function->chunk;
    
    advance();
    
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    
    emitReturn();
    
    // ✅ FIX: Guarda ponteiro antes de resetar
    Function* result = function;
    function = nullptr;  // Evita double-delete
    
    if (hadError) {
        delete result;  // ✅ Liberta se houve erro
        return nullptr;
    }
    
    return result;
}
Function* Compiler::compileExpression(const std::string& source) {
    lexer = new Lexer(source);
    
    function = new Function("__expr__", 0);
    currentChunk = &function->chunk;
    
    advance();
    
    // Check empty expression
    if (check(TOKEN_EOF)) {
        error("Empty expression");
        delete function;
        function = nullptr;
        return nullptr;
    }
    
    expression();
    consume(TOKEN_EOF, "Expect end of expression");
    
    emitByte(OP_RETURN);
    
  
    Function* result = function;
    function = nullptr;
    
    if (hadError) {
        delete result;
        return nullptr;
    }
    
    return result;
}
// ============================================
// TOKEN MANAGEMENT
// ============================================

void Compiler::advance()
{
    previous = current;

    for (;;)
    {
        current = lexer->scanToken();
        if (current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(current.lexeme.c_str());
    }
}

bool Compiler::check(TokenType type)
{
    return current.type == type;
}

bool Compiler::match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

void Compiler::consume(TokenType type, const char *message)
{
    if (current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// ============================================
// ERROR HANDLING
// ============================================

void Compiler::error(const char *message)
{
    errorAt(previous, message);
}

void Compiler::errorAt(Token &token, const char *message)
{
    if (panicMode)
        return;
    panicMode = true;

    fprintf(stderr, "[line %d] Error", token.line);

    if (token.type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token.type == TOKEN_ERROR)
    {
        // Nothing
    }
    else
    {
        fprintf(stderr, " at '%s'", token.lexeme.c_str());
    }

    fprintf(stderr, ": %s\n", message);
    hadError = true;
}

void Compiler::errorAtCurrent(const char *message)
{
    errorAt(current, message);
}

void Compiler::synchronize()
{
    panicMode = false;

    while (current.type != TOKEN_EOF)
    {
        if (previous.type == TOKEN_SEMICOLON)
            return;

        switch (current.type)
        {
        case TOKEN_DEF:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:; // Nothing
        }

        advance();
    }
}

// ============================================
// BYTECODE EMISSION
// ============================================

void Compiler::emitByte(uint8_t byte)
{
    currentChunk->write(byte, previous.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitReturn()
{
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

void Compiler::emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

uint8_t Compiler::makeConstant(Value value)
{
    int constant = currentChunk->addConstant(value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk");
        return 0;
    }

    return (uint8_t)constant;
}

// ============================================
// JUMPS
// ============================================

int Compiler::emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk->count() - 2;
}

void Compiler::patchJump(int offset)
{
    int jump = currentChunk->count() - offset - 2;

    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over");
    }

    currentChunk->code[offset] = (jump >> 8) & 0xff;
    currentChunk->code[offset + 1] = jump & 0xff;
}

void Compiler::emitLoop(int loopStart)
{
    emitByte(OP_LOOP);

    int offset = currentChunk->count() - loopStart + 2;
    if (offset > UINT16_MAX)
    {
        error("Loop body too large");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

// ============================================
// PRATT PARSER - CORE
// ============================================

void Compiler::expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}
 

void Compiler::parsePrecedence(Precedence precedence) {
    advance();
    
    // fprintf(stderr, "DEBUG parsePrecedence: previous=%d, current=%d\n", 
    //         previous.type, current.type);
    
    ParseFn prefixRule = getRule(previous.type)->prefix;
    
    if (prefixRule == nullptr) {
        error("Expect expression");
        return;
    }
    
    bool canAssign = (precedence <= PREC_ASSIGNMENT);
    (this->*prefixRule)(canAssign);
    
    // fprintf(stderr, "DEBUG after prefix: current=%d, prec=%d vs %d\n",
    //         current.type, precedence, getRule(current.type)->prec);
    
    while (precedence <= getRule(current.type)->prec) {
        advance();
        // fprintf(stderr, "DEBUG infix: previous=%d\n", previous.type);
        ParseFn infixRule = getRule(previous.type)->infix;
        (this->*infixRule)(canAssign);
    }
    
    // fprintf(stderr, "DEBUG end parsePrecedence: canAssign=%d, current=%d\n",
    //         canAssign, current.type);
    
    // TU TENS ISTO AQUI? SE SIM, APAGA!
    // if (canAssign && match(TOKEN_EQUAL)) {
    //     error("Invalid assignment target");
    // }
}
ParseRule *Compiler::getRule(TokenType type)
{
    return &rules[type];
}

// ============================================
// PREFIX FUNCTIONS
// ============================================

void Compiler::number(bool canAssign)
{
    if (previous.type == TOKEN_INT)
    {
        int value = std::atoi(previous.lexeme.c_str());
        emitConstant(Value::makeInt(value));
    }
    else
    { // TOKEN_FLOAT
        double value = std::atof(previous.lexeme.c_str());
        emitConstant(Value::makeDouble(value));
    }
}

void Compiler::string(bool canAssign)
{
    const char* interned = StringPool::instance().intern(previous.lexeme);
    emitConstant(Value::makeString(interned));
}

void Compiler::literal(bool canAssign)
{
    switch (previous.type)
    {
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    default:
        return;
    }
}

void Compiler::grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RPAREN, "Expect ')' after expression");
}

void Compiler::unary(bool canAssign)
{
    TokenType operatorType = previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType)
    {
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    default:
        return;
    }
}
void Compiler::binary(bool canAssign) {
    TokenType operatorType = previous.type;
    ParseRule* rule = getRule(operatorType);
    
    parsePrecedence((Precedence)(rule->prec + 1));
    
    switch (operatorType) {
        case TOKEN_PLUS:          emitByte(OP_ADD); break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
        
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:    
            emitByte(OP_EQUAL);
            emitByte(OP_NOT);
            break;
        
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:
            emitByte(OP_GREATER);
            emitByte(OP_NOT);
            break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:
            emitByte(OP_LESS);
            emitByte(OP_NOT);
            break;
        
        default: return;
    }
}

// ============================================
// STATEMENTS (Minimal)
// ============================================

void Compiler::declaration()
{
    if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        statement();
    }

    if (panicMode)
        synchronize();
}

void Compiler::statement()
{
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else
    {
        expressionStatement();
    }
}

void Compiler::printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value");
    emitByte(OP_PRINT);
}

void Compiler::expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression");
    emitByte(OP_POP);
}

// ============================================
// VARIABLES (Stub for Phase 2)
// ============================================

void Compiler::varDeclaration()
{
    error("Variables not implemented yet");
}

void Compiler::variable(bool canAssign)
{
    error("Variables not implemented yet");
}

void Compiler::and_(bool canAssign)
{
    error("Logical operators not implemented yet");
}

void Compiler::or_(bool canAssign)
{
    error("Logical operators not implemented yet");
}

void Compiler::call(bool canAssign)
{
    error("Function calls not implemented yet");
}

// Stubs
uint8_t Compiler::identifierConstant(Token &name) { return 0; }
void Compiler::namedVariable(Token &name, bool canAssign) {}
void Compiler::defineVariable(uint8_t global) {}
void Compiler::declareVariable() {}
void Compiler::addLocal(Token &name) {}
int Compiler::resolveLocal(Token &name) { return -1; }
void Compiler::markInitialized() {}
void Compiler::beginScope() {}
void Compiler::endScope() {}
void Compiler::ifStatement() {}
void Compiler::whileStatement() {}
void Compiler::forStatement() {}
void Compiler::returnStatement() {}
void Compiler::funDeclaration() {}
void Compiler::block() {}