#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdint>

 

class StringPool {
public:
    static StringPool& instance();
    
    // Não copiável
    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;
    
    ~StringPool();
    
    const char* intern(const char* str);
    const char* intern(const std::string& str);
    const char* concat(const char* a, const char* b);
    
    void clear();
    size_t count() const;

private:
    static constexpr size_t BLOCK_SIZE = 64 * 1024;
    
    struct Block {
        char data[BLOCK_SIZE];
        size_t used;
        Block* next;
        
        Block();
    };
    
    StringPool();
    
    void addBlock();
    
    Block* head_;
    Block* current_;
    std::unordered_map<std::string, const char*> interned_;
};
