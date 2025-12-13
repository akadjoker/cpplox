#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdint>

class StringPool
{
public:
    static StringPool &instance()
    {
        static StringPool pool;
        return pool;
    }

    StringPool(const StringPool &) = delete;
    StringPool &operator=(const StringPool &) = delete;

    // Intern string - retorna ponteiro único
    const char *intern(const char *str);
    const char *intern(const std::string &str);

    // Get string ID (para bytecode/globals)
    uint32_t getOrCreateId(const std::string &str);
    const char *getString(uint32_t id) const;

    // Clear (útil para testes)
    void clear();

    // Stats
    size_t size() const { return strings_.size(); }
    size_t uniqueCount() const { return pool_.size(); }

    void dumpStats() const;

private:
    StringPool() = default;
    ~StringPool() { clear(); }

    struct StringEntry
    {
        char *data;
        uint32_t id;
        uint32_t hash;
        size_t refCount;
    };

    // Pool de strings únicas (para interning)
    std::unordered_map<std::string, StringEntry *> pool_;

    // Array de strings por ID (para globals/constants)
    std::vector<const char *> strings_;

    // Mapa inverso: string → ID
    std::unordered_map<std::string, uint32_t> stringToId_;
};
