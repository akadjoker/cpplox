#include "compiler.h"
#include "stringpool.h"
#include "vm.h"
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

Compiler::Compiler(VM *vm)
    : vm_(vm), lexer(nullptr), function(nullptr), currentChunk(nullptr),
      hadError(false), panicMode(false), scopeDepth(0), localCount_(0)
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

    rules[TOKEN_PLUS_PLUS] = {&Compiler::prefixIncrement, nullptr, PREC_NONE};
    rules[TOKEN_MINUS_MINUS] = {&Compiler::prefixDecrement, nullptr, PREC_NONE};

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

Function *Compiler::compile(const std::string &source, VM *vm)
{
    clear();
    vm_ = vm;
    lexer = new Lexer(source);

    function = new Function("__main__", 0);
    currentChunk = &function->chunk;

    advance();

    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    emitReturn();

    Function *result = function;
    function = nullptr;

    if (hadError)
    {
        delete result;
        return nullptr;
    }

    return result;
}

Function *Compiler::compileExpression(const std::string &source, VM *vm)
{
    clear();
    vm_ = vm;
    lexer = new Lexer(source);

    function = new Function("__expr__", 0);
    currentChunk = &function->chunk;

    advance();

    if (check(TOKEN_EOF))
    {
        error("Empty expression");
        delete function;
        function = nullptr;
        return nullptr;
    }

    expression();
    consume(TOKEN_EOF, "Expect end of expression");

    emitByte(OP_RETURN);

    Function *result = function;
    function = nullptr;

    if (hadError)
    {
        delete result;
        return nullptr;
    }

    return result;
}

void Compiler::clear()
{
    delete lexer;
    lexer = nullptr;
    function = nullptr;
    currentChunk = nullptr;
    hadError = false;
    panicMode = false;
    scopeDepth = 0;
    localCount_ = 0;
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

void Compiler::parsePrecedence(Precedence precedence)
{
    advance();

    ParseFn prefixRule = getRule(previous.type)->prefix;

    if (prefixRule == nullptr)
    {
        error("Expect expression");
        return;
    }

    bool canAssign = (precedence <= PREC_ASSIGNMENT);
    (this->*prefixRule)(canAssign);

    while (precedence <= getRule(current.type)->prec)
    {
        advance();
        ParseFn infixRule = getRule(previous.type)->infix;
        (this->*infixRule)(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target");
    }
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
    const char *interned = StringPool::instance().intern(previous.lexeme);
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
void Compiler::binary(bool canAssign)
{
    TokenType operatorType = previous.type;
    ParseRule *rule = getRule(operatorType);

    parsePrecedence((Precedence)(rule->prec + 1));

    switch (operatorType)
    {
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    case TOKEN_PERCENT:
        emitByte(OP_MODULO);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_BANG_EQUAL:
        emitByte(OP_EQUAL);
        emitByte(OP_NOT);
        break;

    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitByte(OP_GREATER);
        emitByte(OP_NOT);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitByte(OP_LESS);
        emitByte(OP_NOT);
        break;

    default:
        return;
    }
}

// ============================================
// STATEMENTS
// ============================================

void Compiler::declaration()
{
    if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else if (match(TOKEN_DEF))
    {
        funDeclaration();
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
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_DO))
    {
        doWhileStatement();
    }
    else if (match(TOKEN_LOOP))
    {
        loopStatement();
    }
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_BREAK))
    {
        breakStatement();
    }
    else if (match(TOKEN_SWITCH))
    {
        switchStatement();
    }
    else if (match(TOKEN_CONTINUE))
    {
        continueStatement();
    }
    else if (match(TOKEN_LBRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else if (match(TOKEN_RETURN))
    {
        returnStatement();
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
// VARIABLES
// ============================================

void Compiler::varDeclaration()
{
    consume(TOKEN_IDENTIFIER, "Expect variable name");
    Token nameToken = previous;

    uint8_t global = identifierConstant(nameToken);

    if (scopeDepth > 0)
    {
        declareVariable();
    }

    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");

    defineVariable(global);
}

void Compiler::variable(bool canAssign)
{
    Token name = previous;

    if (check(TOKEN_LPAREN))
    {
        const char *interned = StringPool::instance().intern(name.lexeme);

        if (vm_->natives_.hasFunction(interned))
        {
            advance(); // Consome '('
            uint8_t argCount = argumentList();
            uint8_t nameIdx = makeConstant(Value::makeString(interned));
            emitBytes(OP_CALL_NATIVE, nameIdx);
            emitByte(argCount);
            return;
        }
    }

    namedVariable(name, canAssign);
}

void Compiler::and_(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

void Compiler::or_(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);

    patchJump(endJump);
}

uint8_t Compiler::identifierConstant(Token &name)
{
    const char *interned = StringPool::instance().intern(name.lexeme);
    return makeConstant(Value::makeString(interned));
}

void Compiler::namedVariable(Token &name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(name);

    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    Token varName = name;

    if (match(TOKEN_PLUS_PLUS))
    {
        // i++ (postfix)
        emitBytes(getOp, (uint8_t)arg);
        emitBytes(getOp, (uint8_t)arg);
        emitConstant(Value::makeInt(1));
        emitByte(OP_ADD);
        emitBytes(setOp, (uint8_t)arg);
        emitByte(OP_POP);
    }
    else if (match(TOKEN_MINUS_MINUS))
    {
        // i-- (postfix)
        emitBytes(getOp, (uint8_t)arg);  // Lê i → Stack: [5]
        emitBytes(getOp, (uint8_t)arg);  // Lê i → Stack: [5, 5]
        emitConstant(Value::makeInt(1)); // Stack: [5, 5, 1]
        emitByte(OP_SUBTRACT);           // Stack: [5, 4]
        emitBytes(setOp, (uint8_t)arg);  // Guarda 4, Stack: [5, 4]
        emitByte(OP_POP);                // Stack: [5]
    }
    else if (canAssign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else if (canAssign && match(TOKEN_PLUS_EQUAL))
    {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_ADD);
        emitBytes(setOp, (uint8_t)arg);
    }
    else if (canAssign && match(TOKEN_MINUS_EQUAL))
    {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_SUBTRACT);
        emitBytes(setOp, (uint8_t)arg);
    }
    else if (canAssign && match(TOKEN_STAR_EQUAL))
    {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_MULTIPLY);
        emitBytes(setOp, (uint8_t)arg);
    }
    else if (canAssign && match(TOKEN_SLASH_EQUAL))
    {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_DIVIDE);
        emitBytes(setOp, (uint8_t)arg);
    }
    else if (canAssign && match(TOKEN_PERCENT_EQUAL))
    {
        emitBytes(getOp, (uint8_t)arg);
        expression();
        emitByte(OP_MODULO);
        emitBytes(setOp, (uint8_t)arg);
    }
    else
    {
        // Leitura normal
        emitBytes(getOp, (uint8_t)arg);
    }
}

void Compiler::defineVariable(uint8_t global)
{
    if (scopeDepth > 0)
    {

        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

void Compiler::declareVariable()
{
    if (scopeDepth == 0)
        return;

    Token &name = previous;

    for (int i = localCount_ - 1; i >= 0; i--)
    {
        Local &local = locals_[i];

        if (local.depth != -1 && local.depth < scopeDepth)
        {
            break;
        }

        if (local.equals(name.lexeme))
        {
            error("Variable with this name already declared in this scope");
        }
    }

    addLocal(name);
}

void Compiler::addLocal(Token &name)
{
    if (localCount_ >= MAX_LOCALS)
    {
        error("Too many local variables in function");
        return;
    }

    size_t len = name.lexeme.length();

    if (len >= MAX_IDENTIFIER_LENGTH)
    {
        error("Identifier name too long (max 31 characters)");
        return;
    }

    // Copia string
    std::memcpy(locals_[localCount_].name, name.lexeme.c_str(), len);
    locals_[localCount_].name[len] = '\0';

    locals_[localCount_].length = (uint8_t)len;
    locals_[localCount_].depth = -1;

    localCount_++;
}

void Compiler::markInitialized()
{
    if (scopeDepth == 0)
        return;
    
    if (localCount_ == 0)
    {
        error("Internal error: marking uninitialized with no locals");
        return;
    }
    locals_[localCount_ - 1].depth = scopeDepth;
}

void Compiler::beginScope()
{
    scopeDepth++;
}

int Compiler::resolveLocal(Token &name)
{
    for (int i = localCount_ - 1; i >= 0; i--)
    {
        Local &local = locals_[i];

        if (local.equals(name.lexeme))
        {
            if (local.depth == -1)
            {
                error("Can't read local variable in its own initializer");
            }
            return i;
        }
    }

    return -1;
}

void Compiler::endScope()
{
    scopeDepth--;

    while (localCount_ > 0 && locals_[localCount_ - 1].depth > scopeDepth)
    {
        emitByte(OP_POP);
        localCount_--;
    }
}

void Compiler::block()
{
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }

    consume(TOKEN_RBRACE, "Expect '}' after block");
}

void Compiler::ifStatement()
{
    // if (condition)
    consume(TOKEN_LPAREN, "Expect '(' after 'if'");
    expression();
    consume(TOKEN_RPAREN, "Expect ')' after condition");

    // Jump para próximo bloco se condição for falsa
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Pop da condição se for true

    // Then branch
    statement();

    // Lista de jumps para o final (depois de cada then/elif executar)
    std::vector<int> endJumps;
    endJumps.push_back(emitJump(OP_JUMP)); // Jump do if

    // Patch do thenJump (aponta para o próximo elif/else/end)
    patchJump(thenJump);
    emitByte(OP_POP); // Pop da condição se for false

    // Elif branches (pode ter vários)
    while (match(TOKEN_ELIF))
    {
        // elif (condition)
        consume(TOKEN_LPAREN, "Expect '(' after 'elif'");
        expression();
        consume(TOKEN_RPAREN, "Expect ')' after elif condition");

        // Jump para próximo bloco se condição for falsa
        int elifJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Pop se elif for true

        // Elif body
        statement();

        // Jump para o final após executar elif
        endJumps.push_back(emitJump(OP_JUMP));

        // Patch do elifJump (aponta para próximo elif/else/end)
        patchJump(elifJump);
        emitByte(OP_POP); // Pop se elif for false
    }

    // Else branch (opcional)
    if (match(TOKEN_ELSE))
    {
        statement();
    }

    // Patch todos os jumps para apontarem para o final
    for (int jump : endJumps)
    {
        patchJump(jump);
    }
}

// void Compiler::ifStatement()
// {
//     // if (condition)
//     consume(TOKEN_LPAREN, "Expect '(' after 'if'");
//     expression();  // Compila a condição
//     consume(TOKEN_RPAREN, "Expect ')' after condition");

//     // Jump para o else/end se a condição for falsa
//     int thenJump = emitJump(OP_JUMP_IF_FALSE);
//     emitByte(OP_POP);  // Pop da condição se for true

//     // Then branch
//     statement();  // Compila o bloco then

//     // Jump para pular o else
//     int elseJump = emitJump(OP_JUMP);

//     // Patch do thenJump (aponta aqui se condição for falsa)
//     patchJump(thenJump);
//     emitByte(OP_POP);  // Pop da condição se for false

//     // Else branch (opcional)
//     if (match(TOKEN_ELSE))
//     {
//         statement();  // Compila o bloco else
//     }

//     // Patch do elseJump (aponta aqui depois do then)
//     patchJump(elseJump);
// }

void Compiler::beginLoop(int loopStart)
{
    LoopContext ctx;
    ctx.loopStart = loopStart;
    ctx.scopeDepth = scopeDepth;
    loopContexts.push_back(ctx);
}

void Compiler::endLoop()
{
    if (loopContexts.empty())
    {
        error("Internal error: endLoop without beginLoop");
        return;
    }

    // Patch todos os breaks para apontarem AQUI (fim do loop)
    LoopContext &ctx = loopContexts.back();
    for (int jump : ctx.breakJumps)
    {
        patchJump(jump);
    }

    loopContexts.pop_back();
}
void Compiler::emitBreak()
{
    if (loopContexts.empty())
    {
        error("Cannot use 'break' outside of a loop");
        return;
    }

    LoopContext &ctx = loopContexts.back();

     while (localCount_ > 0 && locals_[localCount_ - 1].depth > ctx.scopeDepth)
    {
        emitByte(OP_POP);
        localCount_--;
    }
   

    ctx.breakJumps.push_back(emitJump(OP_JUMP));
}

void Compiler::emitContinue()
{
    if (loopContexts.empty())
    {
        error("Cannot use 'continue' outside of a loop");
        return;
    }

    LoopContext &ctx = loopContexts.back();

     while (localCount_ > 0 && locals_[localCount_ - 1].depth > ctx.scopeDepth)
    {
        emitByte(OP_POP);
        localCount_--;
    }

    emitLoop(ctx.loopStart);
}

void Compiler::whileStatement()
{
    int loopStart = currentChunk->count();

    consume(TOKEN_LPAREN, "Expect '(' after 'while'");
    expression();
    consume(TOKEN_RPAREN, "Expect ')' after condition");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    beginLoop(loopStart); // Guarda loopStart SEM scope

    statement(); // Se for {}, o bloco cria o próprio scope

    emitLoop(loopStart);

    endLoop(); // Patch dos breaks

    patchJump(exitJump);
    emitByte(OP_POP);
}
void Compiler::doWhileStatement()
{
    // do
    consume(TOKEN_LBRACE, "Expect '{' after 'do'");

    int loopStart = currentChunk->count();

    beginLoop(loopStart);

    // BODY (executa primeiro)
    beginScope();
    block();
    endScope();

    // while (condition)
    consume(TOKEN_WHILE, "Expect 'while' after do body");
    consume(TOKEN_LPAREN, "Expect '(' after 'while'");
    expression(); // Condição
    consume(TOKEN_RPAREN, "Expect ')' after condition");
    consume(TOKEN_SEMICOLON, "Expect ';' after do-while");

    // Se condição for TRUE, volta ao início
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Pop se true
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP); // Pop se false

    endLoop();
}

void Compiler::loopStatement()
{
    // loop { ... }

    int loopStart = currentChunk->count();

    beginLoop(loopStart);

    // Body
    consume(TOKEN_LBRACE, "Expect '{' after 'loop'");
    beginScope();
    block();
    endScope();

    // Volta sempre ao início (loop infinito)
    emitLoop(loopStart);

    // Patch dos breaks (única forma de sair!)
    endLoop();
}
void Compiler::switchStatement()
{
    consume(TOKEN_LPAREN, "Expect '(' after 'switch'");
    expression(); // Valor do switch fica na stack
    consume(TOKEN_RPAREN, "Expect ')' after switch expression");
    consume(TOKEN_LBRACE, "Expect '{' before switch body");

    // Variável temporária para guardar o valor do switch
    int switchValueSlot = -1;
    
    if (scopeDepth > 0)
    {
        // Local: adiciona variável temporária
        Token temp;
        temp.lexeme = "__switch_temp__";
        addLocal(temp);
        markInitialized();
        
        // ❌ REMOVE:
        // switchValueSlot = locals.size() - 1;
        
        // ✅ ADICIONA:
        switchValueSlot = localCount_ - 1;
        
        emitBytes(OP_SET_LOCAL, (uint8_t)switchValueSlot);
    }
    else
    {
        // Global: cria variável temporária
        const char *tempName = StringPool::instance().intern("__switch_temp__");
        uint8_t globalIdx = makeConstant(Value::makeString(tempName));
        emitBytes(OP_DEFINE_GLOBAL, globalIdx);
    }
    
    std::vector<int> caseEndJumps;
    int defaultStart = -1;
    bool hasDefault = false;

    // Parse todos os cases
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF))
    {
        if (match(TOKEN_CASE))
        {
            // case VALUE:
            
            // Carrega valor do switch
            if (switchValueSlot != -1)
            {
                emitBytes(OP_GET_LOCAL, (uint8_t)switchValueSlot);
            }
            else
            {
                const char *tempName = StringPool::instance().intern("__switch_temp__");
                uint8_t globalIdx = makeConstant(Value::makeString(tempName));
                emitBytes(OP_GET_GLOBAL, globalIdx);
            }

            // Valor do case
            expression();
            consume(TOKEN_COLON, "Expect ':' after case value");

            // Compara
            emitByte(OP_EQUAL);

            // Se NÃO for igual, pula este case
            int skipCase = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP);

            // Executa statements do case
            while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
                   !check(TOKEN_RBRACE) && !check(TOKEN_EOF))
            {
                statement();
            }

            // Break implícito - pula para o fim
            caseEndJumps.push_back(emitJump(OP_JUMP));

            // Se não era igual, pula para aqui (próximo case)
            patchJump(skipCase);
            emitByte(OP_POP);
        }
        else if (match(TOKEN_DEFAULT))
        {
            // default:
            consume(TOKEN_COLON, "Expect ':' after 'default'");
            
            if (hasDefault)
            {
                error("Switch can only have one 'default' case");
            }
            hasDefault = true;

            // Executa statements
            while (!check(TOKEN_CASE) && !check(TOKEN_RBRACE) && !check(TOKEN_EOF))
            {
                statement();
            }
        }
        else
        {
            errorAtCurrent("Expect 'case' or 'default' in switch body");
            break;
        }
    }

    consume(TOKEN_RBRACE, "Expect '}' after switch body");

    // Patch todos os jumps de fim de case
    for (int jump : caseEndJumps)
    {
        patchJump(jump);
    }

    // Limpa variável temporária do switch
    if (switchValueSlot != -1)
    {
        // Local - já vai ser limpo pelo endScope se necessário
    }
    else
    {
        // Global - podemos deixar (ou limpar )
    }
}

void Compiler::breakStatement()
{
    emitBreak();
    consume(TOKEN_SEMICOLON, "Expect ';' after 'break'");
}

void Compiler::continueStatement()
{
    emitContinue();
    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'");
}

void Compiler::forStatement()
{
    // for cria um scope próprio para o initializer
    beginScope();

    consume(TOKEN_LPAREN, "Expect '(' after 'for'");

    // INITIALIZER (opcional)
    // Pode ser: var i = 0; ou i = 0; ou vazio
    if (match(TOKEN_SEMICOLON))
    {
        // Sem initializer
    }
    else if (match(TOKEN_VAR))
    {
        varDeclaration(); // var i = 0;
    }
    else
    {
        expressionStatement(); // i = 0;
    }

    // Marca onde o loop começa (para continue e para o loop)
    int loopStart = currentChunk->count();

    // CONDITION (opcional)
    int exitJump = -1;
    if (!check(TOKEN_SEMICOLON))
    {
        expression(); // i < 10
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");

        // Pula para fora se condição for falsa
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Pop da condição
    }
    else
    {
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");
    }

    // INCREMENT (opcional)
    // Problema: increment vem ANTES do body no código, mas executa DEPOIS
    // Solução: pular o increment, executar body, depois voltar pro increment
    if (!check(TOKEN_RPAREN))
    {
        // Pula sobre o código do increment
        int bodyJump = emitJump(OP_JUMP);

        int incrementStart = currentChunk->count();
        expression();     // i = i + 1
        emitByte(OP_POP); // Pop do resultado
        consume(TOKEN_RPAREN, "Expect ')' after for clauses");

        // Volta para o início do loop (condition)
        emitLoop(loopStart);

        // Agora loopStart aponta para o increment (para continue)
        loopStart = incrementStart;

        // Patch do bodyJump para pular o increment
        patchJump(bodyJump);
    }
    else
    {
        consume(TOKEN_RPAREN, "Expect ')' after for clauses");
    }

    // Registra o loop para break/continue
    beginLoop(loopStart);

    // BODY
    statement();

    // Volta para o increment (ou condition se não houver increment)
    emitLoop(loopStart);

    // Patch do exitJump (se houver condition)
    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP); // Pop da condição falsa
    }

    endLoop();  // Patch dos breaks
    endScope(); // Limpa variáveis do initializer
}

void Compiler::returnStatement()
{

    if (function == nullptr)
    {
        error("Can't return from top-level code");
        return;
    }

    if (match(TOKEN_SEMICOLON))
    {
        // return;
        emitByte(OP_RETURN_NIL);
    }
    else
    {
        // return <expr>;
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value");
        emitByte(OP_RETURN);
    }

    function->hasReturn = true;
}

uint8_t Compiler::argumentList()
{
    uint8_t argCount = 0;

    if (!check(TOKEN_RPAREN))
    {
        do
        {
            expression();

            if (argCount == 255)
            {
                error("Can't have more than 255 arguments");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RPAREN, "Expect ')' after arguments");
    return argCount;
}

void Compiler::call(bool canAssign)
{
    uint8_t argCount = argumentList();
    emitByte(OP_CALL);
    emitByte(argCount);
}

void Compiler::funDeclaration()
{
    // 1. Parse nome
    consume(TOKEN_IDENTIFIER, "Expect function name");

    Token nameToken = previous;

    if (!vm_->canRegisterFunction(nameToken.lexeme))
    {
        error("Function with this name already registered");
        return;
    }

    uint8_t nameConstant = identifierConstant(nameToken);

    // 2. Declare variable (antes de compilar o corpo!)
    if (scopeDepth > 0)
    {
        declareVariable();
    }

    // 3. Compile a função
    compileFunction(nameToken.lexeme);

    // 4. Define variable
    defineVariable(nameConstant);
}
void Compiler::compileFunction(const std::string &name)
{
    if (!vm_->canRegisterFunction(name))
    {
        error("Function with this name already registered");
        return;
    }
    
    Function *function = new Function(name, 0);
    uint16_t idx = vm_->registerFunction(name, function);

    // Salvar estado do compiler atual
    Function *enclosingFunction = this->function;
    Chunk *enclosingChunk = this->currentChunk;
    int enclosingScopeDepth = this->scopeDepth;
    
 
    Local enclosingLocals[MAX_LOCALS];
    int enclosingLocalCount = this->localCount_;
    std::memcpy(enclosingLocals, locals_, sizeof(Local) * localCount_);

    // Mudar para compilar a função
    this->function = function;
    this->currentChunk = &function->chunk;
    this->scopeDepth = 0;
    
 
    this->localCount_ = 0;
    
    function->hasReturn = false;

    // Parse parâmetros
    beginScope();
    
    consume(TOKEN_LPAREN, "Expect '(' after function name");
    
    if (!check(TOKEN_RPAREN))
    {
        do
        {
            function->arity++;
            if (function->arity > 255)
            {
                error("Can't have more than 255 parameters");
                break;
            }
            
            consume(TOKEN_IDENTIFIER, "Expect parameter name");
            addLocal(previous);
            markInitialized();
            
        } while (match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RPAREN, "Expect ')' after parameters");

    // Parse corpo
    consume(TOKEN_LBRACE, "Expect '{' before function body");
    block();
    
    if (!function->hasReturn)
    {
        emitReturn();
    }

    // Restaurar estado do compiler
    this->function = enclosingFunction;
    this->currentChunk = enclosingChunk;
    this->scopeDepth = enclosingScopeDepth;
    
 
    this->localCount_ = enclosingLocalCount;
    std::memcpy(locals_, enclosingLocals, sizeof(Local) * localCount_);

    emitBytes(OP_CONSTANT, makeConstant(Value::makeFunction(idx)));
}


void Compiler::prefixIncrement(bool canAssign)
{
    // ++i
    // previous = '++', current deve ser identifier

    if (!check(TOKEN_IDENTIFIER))
    {
        error("Expect variable name after '++'");
        return;
    }

    advance();             // Consome o identifier manualmente
    Token name = previous; // Agora previous é o identifier

    uint8_t getOp, setOp;
    int arg = resolveLocal(name);

    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    // i = i + 1
    emitBytes(getOp, (uint8_t)arg);
    emitConstant(Value::makeInt(1));
    emitByte(OP_ADD);
    emitBytes(setOp, (uint8_t)arg);

    // Lê o novo valor para retornar
    emitBytes(getOp, (uint8_t)arg);
}

void Compiler::prefixDecrement(bool canAssign)
{
    // --i

    if (!check(TOKEN_IDENTIFIER))
    {
        error("Expect variable name after '--'");
        return;
    }

    advance();
    Token name = previous;

    uint8_t getOp, setOp;
    int arg = resolveLocal(name);

    if (arg != -1)
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    emitBytes(getOp, (uint8_t)arg);
    emitConstant(Value::makeInt(1));
    emitByte(OP_SUBTRACT);
    emitBytes(setOp, (uint8_t)arg);

    emitBytes(getOp, (uint8_t)arg);
}
