#include "stringpool.h"
#include <cstdio>

#include "stringpool.h"

// ============================================
// Block
// ============================================
StringPool::Block::Block() : used(0), next(nullptr) {}

// ============================================
// Singleton
// ============================================
StringPool &StringPool::instance()
{
    static StringPool inst;
    return inst;
}

StringPool::StringPool() : head_(nullptr), current_(nullptr)
{
    addBlock();
}

StringPool::~StringPool()
{
    Block *b = head_;
    while (b)
    {
        Block *next = b->next;
        delete b;
        b = next;
    }
}

// ============================================
// Intern
// ============================================
const char *StringPool::intern(const char *str)
{
    size_t len = strlen(str);

    // Lookup
    auto it = interned_.find(str);
    if (it != interned_.end())
    {
        return it->second;
    }

    // Aloca na arena
    size_t needed = len + 1;
    if (current_->used + needed > BLOCK_SIZE)
    {
        addBlock();
    }

    char *ptr = current_->data + current_->used;
    memcpy(ptr, str, len);
    ptr[len] = '\0';
    current_->used += needed;

    // Guarda no map
    interned_[std::string(ptr, len)] = ptr;

    return ptr;
}

const char *StringPool::intern(const std::string &str)
{
    return intern(str.c_str());
}

// ============================================
// Concat
// ============================================
const char *StringPool::concat(const char *a, const char *b)
{
    size_t lenA = strlen(a);
    size_t lenB = strlen(b);
    size_t total = lenA + lenB;

    // Checa se já existe
    std::string key;
    key.reserve(total);
    key.append(a, lenA);
    key.append(b, lenB);

    auto it = interned_.find(key);
    if (it != interned_.end())
    {
        return it->second;
    }

    // Aloca
    size_t needed = total + 1;
    if (current_->used + needed > BLOCK_SIZE)
    {
        addBlock();
    }

    char *ptr = current_->data + current_->used;
    memcpy(ptr, a, lenA);
    memcpy(ptr + lenA, b, lenB);
    ptr[total] = '\0';
    current_->used += needed;

    interned_[std::move(key)] = ptr;

    return ptr;
}

// ============================================
// Utils
// ============================================
void StringPool::clear()
{
    if (head_)
    {
        Block *b = head_->next;
        while (b)
        {
            Block *next = b->next;
            delete b;
            b = next;
        }
        head_->next = nullptr;
        head_->used = 0;
        current_ = head_;
    }
    interned_.clear();
}

size_t StringPool::count() const
{
    return interned_.size();
}

void StringPool::addBlock()
{
    Block *b = new Block();
    if (!head_)
    {
        head_ = b;
    }
    else
    {
        current_->next = b;
    }
    current_ = b;
}
