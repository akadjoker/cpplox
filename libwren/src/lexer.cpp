#include "lexer.h"
#include <cctype>
#include <iostream>
Lexer::Lexer(const std::string &src)
    : source(src),
      start(0),
      current(0),
      line(1),
      column(1),
      tokenColumn(1),
      hasPendingError(false),
      pendingErrorMessage(""),
      pendingErrorLine(0),
      pendingErrorColumn(0)
{
    initKeywords();
}

void Lexer::setPendingError(const std::string &message)
{
    if (!hasPendingError)
    {
        hasPendingError = true;
        pendingErrorMessage = message;
        pendingErrorLine = line;
        pendingErrorColumn = column;
    }
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
        {"do", TOKEN_DO},
        {"loop", TOKEN_LOOP},
        {"switch", TOKEN_SWITCH},
        {"case", TOKEN_CASE},
        {"default", TOKEN_DEFAULT},
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

// void Lexer::skipWhitespace()
// {
//     while (!isAtEnd())
//     {
//         char c = peek();

//         switch (c)
//         {
//         case ' ':
//         case '\r':
//         case '\t':
//         case '\n':
//             advance();
//             break;

//         case '/':
//             if (peekNext() == '/')
//             {
//                 // Line comment
//                 while (peek() != '\n' && !isAtEnd())
//                 {
//                     advance();
//                 }
//             }
//             else if (peekNext() == '*')
//             {
//                 // Block comment
//                 advance(); // /
//                 advance(); // *

//                 int nestLevel = 1;

//                 while (!isAtEnd() && nestLevel > 0)
//                 {
//                     if (peek() == '/' && peekNext() == '*')
//                     {
//                         advance();
//                         advance();
//                         nestLevel++;
//                     }
//                     else if (peek() == '*' && peekNext() == '/')
//                     {
//                         advance();
//                         advance();
//                         nestLevel--;
//                     }
//                     else
//                     {
//                         advance();
//                     }
//                 }

//                 // If we hit EOF with unclosed comment, that's an error
//                 // but we'll handle it in scanToken() by returning TOKEN_ERROR
//                 if (nestLevel > 0)
//                 {
//                     // Don't set pending error, just return
//                     // scanToken() will detect EOF
//                     return;
//                 }
//             }
//             else
//             {
//                 return; // It's the / operator
//             }
//             break;

//         default:
//             return;
//         }
//     }
// }

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
            advance();
            break;

        case '\n':
            advance();
            break;

        case '/':
            if (peekNext() == '/')
            {
                // Comentário linha
                advance();
                advance();
                while (peek() != '\n' && !isAtEnd())
                {
                    advance();
                }
            }
            else if (peekNext() == '*')
            {
                advance(); // /
                advance(); // *

                bool closed = false;
                size_t commentStart = current;
                const size_t MAX_COMMENT_LENGTH = 100000;

                while (!isAtEnd())
                {
                    if (current - commentStart > MAX_COMMENT_LENGTH)
                    {
                        setPendingError("Comment too long (max 100k chars)");
                        while (!isAtEnd() && !(peek() == '*' && peekNext() == '/'))
                        {
                            advance();
                        }
                        if (peek() == '*')
                        {
                            advance();
                            advance();
                        }
                        return; // Sai para reportar erro
                    }

                    if (peek() == '*' && peekNext() == '/')
                    {
                        advance(); // *
                        advance(); // /
                        closed = true;
                        break;
                    }

                    advance();
                }

                if (!closed && isAtEnd())
                {
                    setPendingError("Unterminated block comment");
                    //  Não há mais nada a fazer, já estamos no fim
                }
            }
            else
            {
                return; // É operador /, não comentário
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

Token Lexer::string()
{
    const size_t MAX_STRING_LENGTH = 10000;
    size_t startPos = current;

    while (peek() != '"' && !isAtEnd())
    {
        advance();

        if (current - startPos > MAX_STRING_LENGTH)
        {
            return errorToken("String too long (max 10000 chars)");
        }
    }

    if (isAtEnd())
    {
        return errorToken("Unterminated string");
    }

    advance(); // fecha "

    std::string value = source.substr(start + 1, current - start - 2);
    return makeToken(TOKEN_STRING, value);
}


Token Lexer::identifier()
{
    const size_t MAX_IDENTIFIER_LENGTH = 255;
    size_t startPos = current - 1;

    while (isalnum(peek()) || peek() == '_')
    {
        advance();

        if (current - startPos > MAX_IDENTIFIER_LENGTH)
        {
            return errorToken("Identifier too long (max 255 chars)");
        }
    }

    std::string text = source.substr(start, current - start);

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
    case ':':
        return makeToken(TOKEN_COLON, ":");
    
    // Operators com compound assignment e increment/decrement
    case '+':
        if (match('+'))
            return makeToken(TOKEN_PLUS_PLUS, "++");
        if (match('='))
            return makeToken(TOKEN_PLUS_EQUAL, "+=");
        return makeToken(TOKEN_PLUS, "+");
    
    case '-':
        if (match('-'))
            return makeToken(TOKEN_MINUS_MINUS, "--");
        if (match('='))
            return makeToken(TOKEN_MINUS_EQUAL, "-=");
        return makeToken(TOKEN_MINUS, "-");
    
    case '*':
        if (match('='))
            return makeToken(TOKEN_STAR_EQUAL, "*=");
        return makeToken(TOKEN_STAR, "*");
    
    case '/':
        if (match('='))
            return makeToken(TOKEN_SLASH_EQUAL, "/=");
        return makeToken(TOKEN_SLASH, "/");
    
    case '%':
        if (match('='))
            return makeToken(TOKEN_PERCENT_EQUAL, "%=");
        return makeToken(TOKEN_PERCENT, "%");
    
    // Two-char tokens
    case '=':
        if (match('='))
        {
            return makeToken(TOKEN_EQUAL_EQUAL, "==");
        }
        return makeToken(TOKEN_EQUAL, "=");
    
    case '!':
        if (match('='))
        {
            return makeToken(TOKEN_BANG_EQUAL, "!=");
        }
        return makeToken(TOKEN_BANG, "!");
    
    case '<':
        if (match('='))
        {
            return makeToken(TOKEN_LESS_EQUAL, "<=");
        }
        return makeToken(TOKEN_LESS, "<");
    
    case '>':
        if (match('='))
        {
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
Token Lexer::nextToken()
{

    if (hasPendingError)
    {
        Token errorTok(TOKEN_ERROR, pendingErrorMessage,
                       pendingErrorLine, pendingErrorColumn);
        hasPendingError = false; // Limpa erro
        return errorTok;
    }

    skipWhitespace();

    if (hasPendingError)
    {
        Token errorTok(TOKEN_ERROR, pendingErrorMessage,
                       pendingErrorLine, pendingErrorColumn);
        hasPendingError = false;
        return errorTok;
    }

    start = current;
    tokenColumn = column;

    return scanToken();
}

// Token Lexer::scanToken()
// {

//     skipWhitespace();

//     start = current;
//     tokenColumn = column;

//     if (isAtEnd())
//     {
//         return makeToken(TOKEN_EOF, "");
//     }

//     char c = advance();

//     // Numbers
//     if (isdigit(c))
//     {
//         return number();
//     }

//     // Identifiers and keywords
//     if (isalpha(c) || c == '_')
//     {
//         return identifier();
//     }

//     switch (c)
//     {
//     // Single-char tokens
//     case '(':
//         return makeToken(TOKEN_LPAREN, "(");
//     case ')':
//         return makeToken(TOKEN_RPAREN, ")");
//     case '{':
//         return makeToken(TOKEN_LBRACE, "{");
//     case '}':
//         return makeToken(TOKEN_RBRACE, "}");
//     case ',':
//         return makeToken(TOKEN_COMMA, ",");
//     case ';':
//         return makeToken(TOKEN_SEMICOLON, ";");
//     case '+':
//         return makeToken(TOKEN_PLUS, "+");
//     case '-':
//         return makeToken(TOKEN_MINUS, "-");
//     case '*':
//         return makeToken(TOKEN_STAR, "*");
//     case '%':
//         return makeToken(TOKEN_PERCENT, "%");
//     case '/':
//         return makeToken(TOKEN_SLASH, "/");

//     // Two-char tokens
//     case '=':
//         if (match('='))
//         {
//             return makeToken(TOKEN_EQUAL_EQUAL, "==");
//         }
//         return makeToken(TOKEN_EQUAL, "=");

//     case '!':
//         if (match('='))
//         {
//             return makeToken(TOKEN_BANG_EQUAL, "!=");
//         }
//         return makeToken(TOKEN_BANG, "!");

//     case '<':
//         if (match('='))
//         {
//             return makeToken(TOKEN_LESS_EQUAL, "<=");
//         }
//         return makeToken(TOKEN_LESS, "<");

//     case '>':
//         if (match('='))
//         {
//             return makeToken(TOKEN_GREATER_EQUAL, ">=");
//         }
//         return makeToken(TOKEN_GREATER, ">");

//     case '&':
//         if (match('&'))
//         {
//             return makeToken(TOKEN_AND_AND, "&&");
//         }
//         return errorToken("Expected '&&' for logical AND");

//     case '|':
//         if (match('|'))
//         {
//             return makeToken(TOKEN_OR_OR, "||");
//         }
//         return errorToken("Expected '||' for logical OR");

//     // String literals
//     case '"':
//         return string();

//     default:
//         return errorToken("Unexpected character");
//     }
// }

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
        token = nextToken();
        tokens.push_back(token);

    } while (token.type != TOKEN_EOF && !hasPendingError);

    return tokens;
}

void Lexer::printTokens(const std::vector<Token> &toks) const
{
    for (const Token &token : toks)
    {
        std::cout << token.toString() << std::endl;
    }
}