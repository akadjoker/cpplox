#pragma once

#include "token.h"
#include <vector>
#include <unordered_map>
#include <string>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    
    Token nextToken();
    std::vector<Token> scanTokens();
    void printTokens(const std::vector<Token>& tokens) const;
    void reset();
    
private:
    std::string source;
    
    size_t start;
    size_t current;
    int line;
    int column;
    int tokenColumn;
    
    // ✅ NOVO: Flag de erro pendente
    bool hasPendingError;
    std::string pendingErrorMessage;
    int pendingErrorLine;
    int pendingErrorColumn;
    
    std::unordered_map<std::string, TokenType> keywords;
    
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    void skipWhitespace();
    
    Token scanToken();
    Token makeToken(TokenType type, const std::string& lexeme);
    
    Token number();
    Token string();
    Token identifier();
    
    Token errorToken(const std::string& message);
    
 
    void setPendingError(const std::string& message);
    
    void initKeywords();
};
