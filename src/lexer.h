#pragma once

#include "token.h"
#include <vector>
#include <unordered_map>
#include <string>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    
 
    Token scanToken();
    
 
    std::vector<Token> scanAll();
    void printTokens(const std::vector<Token>& tokens) const;
    
    void reset();
    
private:
    std::string source;
    
    size_t start;
    size_t current;
    int line;
    int column;
    int tokenColumn;
    
    std::unordered_map<std::string, TokenType> keywords;
    
    // Helper methods
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    void skipWhitespace();
    
    Token makeToken(TokenType type, const std::string& lexeme);
    Token errorToken(const std::string& message);
    
    // Token scanners
    Token number();
    Token string();
    Token identifier();
    
    void initKeywords();
};