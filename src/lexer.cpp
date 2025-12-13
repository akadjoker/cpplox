#include "lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(const std::string &src)
    : source(src),
      start(0),
      current(0),
      line(1),
      column(1),
      tokenColumn(1)
{
    initKeywords();
}

void Lexer::initKeywords()
{
    keywords = {
        {"var", TOKEN_VAR},
        {"def", TOKEN_DEF},
        {"if", TOKEN_IF},
        {"elif", TOKEN_ELIF},
        {"else", TOKEN_ELSE},
        {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR},
        {"return", TOKEN_RETURN},
        {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE},
        {"true", TOKEN_TRUE},
        {"false", TOKEN_FALSE},
        {"nil", TOKEN_NIL},
        {"print", TOKEN_PRINT},
        {"type", TOKEN_TYPE},
    };
}

void Lexer::reset()
{
    start = 0;
    current = 0;
    line = 1;
    column = 1;
    tokenColumn = 1;
}

bool Lexer::isAtEnd() const
{
    return current >= source.length();
}

char Lexer::advance()
{
    if (isAtEnd())
        return '\0';

    char c = source[current++];

    if (c == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }

    return c;
}

char Lexer::peek() const
{
    if (isAtEnd())
        return '\0';
    return source[current];
}

char Lexer::peekNext() const
{
    if (current + 1 >= source.length())
        return '\0';
    return source[current + 1];
}

bool Lexer::match(char expected)
{
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;
    advance();
    return true;
}

void Lexer::skipWhitespace()
{
    while (!isAtEnd())
    {
        char c = peek();

        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            advance();
            break;

        case '/':
            if (peekNext() == '/')
            {
                // Line comment
                while (peek() != '\n' && !isAtEnd())
                {
                    advance();
                }
            }
            else if (peekNext() == '*')
            {
                // Block comment
                advance(); // /
                advance(); // *

                int nestLevel = 1;

                while (!isAtEnd() && nestLevel > 0)
                {
                    if (peek() == '/' && peekNext() == '*')
                    {
                        advance();
                        advance();
                        nestLevel++;
                    }
                    else if (peek() == '*' && peekNext() == '/')
                    {
                        advance();
                        advance();
                        nestLevel--;
                    }
                    else
                    {
                        advance();
                    }
                }

                // If we hit EOF with unclosed comment, that's an error
                // but we'll handle it in scanToken() by returning TOKEN_ERROR
                if (nestLevel > 0)
                {
                    // Don't set pending error, just return
                    // scanToken() will detect EOF
                    return;
                }
            }
            else
            {
                return; // It's the / operator
            }
            break;

        default:
            return;
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string &lexeme)
{
    return Token(type, lexeme, line, tokenColumn);
}

Token Lexer::errorToken(const std::string &message)
{
    return Token(TOKEN_ERROR, message, line, tokenColumn);
}

Token Lexer::number()
{
    while (isdigit(peek()))
    {
        advance();
    }

    TokenType type = TOKEN_INT;

    if (peek() == '.' && isdigit(peekNext()))
    {
        type = TOKEN_FLOAT;
        advance(); // consume '.'

        while (isdigit(peek()))
        {
            advance();
        }
    }

    std::string numStr = source.substr(start, current - start);
    return makeToken(type, numStr);
}

// lexer.cpp - Token Lexer::string()
Token Lexer::string()
{
    // fprintf(stderr, "DEBUG string(): start=%zu, current=%zu\n", start, current);
    // fprintf(stderr, "DEBUG char at start: '%c'\n", source[start]);
    // fprintf(stderr, "DEBUG char at current: '%c'\n", source[current]);

    size_t stringStart = current;

    while (peek() != '"' && !isAtEnd())
    {
        advance();
    }

    if (isAtEnd())
    {
        return errorToken("Unterminated string");
    }

  // fprintf(stderr, "DEBUG before capture: stringStart=%zu, current=%zu\n", stringStart, current);
    std::string value = source.substr(stringStart, current - stringStart);
    //fprintf(stderr, "DEBUG captured: '%s'\n", value.c_str());

    advance(); // Consome " final

    return makeToken(TOKEN_STRING, value);
}

Token Lexer::identifier()
{
    while (isalnum(peek()) || peek() == '_')
    {
        advance();
    }

    std::string text = source.substr(start, current - start);

    // Check if it's a keyword
    auto it = keywords.find(text);
    if (it != keywords.end())
    {
        return makeToken(it->second, text);
    }

    return makeToken(TOKEN_IDENTIFIER, text);
}

// ============================================
// MAIN API: scanToken()
// ============================================

Token Lexer::scanToken()
{
    skipWhitespace();

    start = current;
    tokenColumn = column;

    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF, "");
    }

    char c = advance();

    // Numbers
    if (isdigit(c))
    {
        return number();
    }

    // Identifiers and keywords
    if (isalpha(c) || c == '_')
    {
        return identifier();
    }

    switch (c)
    {
    // Single-char tokens
    case '(':
        return makeToken(TOKEN_LPAREN, "(");
    case ')':
        return makeToken(TOKEN_RPAREN, ")");
    case '{':
        return makeToken(TOKEN_LBRACE, "{");
    case '}':
        return makeToken(TOKEN_RBRACE, "}");
    case ',':
        return makeToken(TOKEN_COMMA, ",");
    case ';':
        return makeToken(TOKEN_SEMICOLON, ";");
    case '+':
        return makeToken(TOKEN_PLUS, "+");
    case '-':
        return makeToken(TOKEN_MINUS, "-");
    case '*':
        return makeToken(TOKEN_STAR, "*");
    case '%':
        return makeToken(TOKEN_PERCENT, "%");
    case '/':
        return makeToken(TOKEN_SLASH, "/");

    // Two-char tokens
    case '=':
        if (match('=')) {
            return makeToken(TOKEN_EQUAL_EQUAL, "==");
        }
        return makeToken(TOKEN_EQUAL, "=");

    case '!':
        if (match('=')) {
            return makeToken(TOKEN_BANG_EQUAL, "!=");
        }
        return makeToken(TOKEN_BANG, "!");

    case '<':
        if (match('=')) {
            return makeToken(TOKEN_LESS_EQUAL, "<=");
        }
        return makeToken(TOKEN_LESS, "<");

    case '>':
        if (match('=')) {
            return makeToken(TOKEN_GREATER_EQUAL, ">=");
        }
        return makeToken(TOKEN_GREATER, ">");

    case '&':
        if (match('&'))
        {
            return makeToken(TOKEN_AND_AND, "&&");
        }
        return errorToken("Expected '&&' for logical AND");

    case '|':
        if (match('|'))
        {
            return makeToken(TOKEN_OR_OR, "||");
        }
        return errorToken("Expected '||' for logical OR");

    // String literals
    case '"':
        return string();

    default:
        return errorToken("Unexpected character");
    }
}

// ============================================
// BATCH API: For tools/debugging
// ============================================

std::vector<Token> Lexer::scanAll()
{
    std::vector<Token> tokens;
    tokens.reserve(256); // Pre-allocate for performance

    Token token;
    do
    {
        token = scanToken();
        tokens.push_back(token);

    } while (token.type != TOKEN_EOF);

    return tokens;
}

void Lexer::printTokens(const std::vector<Token> &toks) const
{
    for (const Token &token : toks)
    {
        std::cout << token.toString() << std::endl;
    }
}