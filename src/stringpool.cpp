#include "stringpool.h"
#include <cstdio>

const char *StringPool::intern(const char *str)
{
    return intern(std::string(str));
}

const char *StringPool::intern(const std::string &str)
{
    // Já existe no pool?
    auto it = pool_.find(str);
    if (it != pool_.end())
    {
        it->second->refCount++;
        return it->second->data;
    }

    // Cria nova string
    StringEntry *entry = new StringEntry;
    entry->data = new char[str.length() + 1];
    std::strcpy(entry->data, str.c_str());
    entry->hash = std::hash<std::string>{}(str);
    entry->refCount = 1;
    entry->id = static_cast<uint32_t>(strings_.size());

    pool_[str] = entry;
    strings_.push_back(entry->data);
    stringToId_[str] = entry->id;

    return entry->data;
}

uint32_t StringPool::getOrCreateId(const std::string &str)
{
    // Já tem ID?
    auto it = stringToId_.find(str);
    if (it != stringToId_.end())
    {
        return it->second;
    }

    // Intern e retorna ID
    intern(str);
    return stringToId_[str];
}

const char *StringPool::getString(uint32_t id) const
{
    if (id >= strings_.size())
    {
        return "";
    }
    return strings_[id];
}

void StringPool::clear()
{
    for (auto &[key, entry] : pool_)
    {
        delete[] entry->data;
        delete entry;
    }
    pool_.clear();
    strings_.clear();
    stringToId_.clear();
}

void StringPool::dumpStats() const
{
    printf("=== String Pool Stats ===\n");
    printf("  Unique strings: %zu\n", pool_.size());
    printf("  Total IDs:      %zu\n", strings_.size());
    printf("  Memory saved:   ~%zu bytes (estimated)\n",
           (strings_.size() - pool_.size()) * 10);
    printf("========================\n");
}