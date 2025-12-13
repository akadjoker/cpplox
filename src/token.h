#pragma once

#include <string>
#include <cstdint>

enum TokenType
{
    // Literals
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,

    // Keywords
    TOKEN_VAR,
    TOKEN_DEF,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    // Operators - Arithmetic
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    

    // Operators - Comparison
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,

    // Operators - Logical
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_BANG,

    // Delimiters
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    // Built-ins
    TOKEN_PRINT,
    TOKEN_TYPE,

    // Special
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_COUNT 
};

struct Token
{
    TokenType type;
    std::string lexeme;

 
    int line;   // Linha (1-indexed)
    int column; // Coluna (1-indexed)

    Token();

    Token(TokenType t, const std::string &lex, int l, int c);

 
    std::string toString() const;
    std::string locationString() const; // "line 5, column 12"
};

 
const char *tokenTypeToString(TokenType type);
