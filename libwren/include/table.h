#pragma once

#include "value.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>

#define MAX_VAR_NAME_LENGTH 32 // ← Mesmo que Local

class Table
{
private:
    // ========================================================================
    // ARRAY PART - Dense storage for numeric indices
    // ========================================================================
    Value *array;
    size_t array_size;
    size_t array_capacity;

    // ========================================================================
    // HASH PART - Open addressing with linear probing
    // ========================================================================
    struct HashNode
    {
        char key[MAX_VAR_NAME_LENGTH];
        Value value;
        bool occupied;

        HashNode() : occupied(false)
        {
            key[0] = '\0';
            value.type = VAL_NULL;
        }

        // Helper: seta key de forma segura
        void set_key(const char *str)
        {
            size_t len = strlen(str);
            if (len >= MAX_VAR_NAME_LENGTH)
            {
                len = MAX_VAR_NAME_LENGTH - 1;
            }
            memcpy(key, str, len);
            key[len] = '\0';
        }

        // Helper: compara key
        bool key_equals(const char *str) const
        {
            return strcmp(key, str) == 0;
        }
    };

    HashNode *hash_buckets;
    size_t hash_size;
    size_t hash_capacity;

    static const size_t INITIAL_ARRAY_CAPACITY = 16;
    static const size_t INITIAL_HASH_CAPACITY = 32;
    static const size_t MAX_LOAD_PERCENT = 75;

    // ========================================================================
    // HASH FUNCTION - SIMD-friendly FNV-1a
    // ========================================================================
    size_t hash_string(const char *str) const
    {
        size_t h = 14695981039346656037ULL;
        const char *p = str;

        // Process 8 bytes at a time when possible
        while (*p && *(p + 7))
        {
            h = (h ^ *(uint64_t *)p) * 1099511628211ULL;
            p += 8;
        }

        // Finish remaining bytes
        while (*p)
            h = (h ^ *p++) * 1099511628211ULL;

        return h;
    }

    // ========================================================================
    // FIND SLOT - Single-pass lookup with protection
    // ========================================================================
    size_t find_hash_slot(const char *key) const
    {
        if (hash_capacity == 0)
            return 0;

        size_t h = hash_string(key);
        size_t slot = h & (hash_capacity - 1);
        size_t original_slot = slot;

        // Protected probing - checks for full rotation
        do
        {
            if (!hash_buckets[slot].occupied)
                return slot;

            if (strcmp(hash_buckets[slot].key, key) == 0)
                return slot;

            slot = (slot + 1) & (hash_capacity - 1);
        } while (slot != original_slot);

        return original_slot; // Should never reach here with load factor < 1.0
    }

    // ========================================================================
    // RESIZE ARRAY - Pre-allocate for known workloads
    // ========================================================================
    void resize_array(size_t new_capacity)
    {
        Value *new_array = (Value *)calloc(new_capacity, sizeof(Value));

        for (size_t i = 0; i < new_capacity; ++i)
            new_array[i].type = VAL_NULL;

        if (array)
        {
            size_t copy_count = (array_size < new_capacity) ? array_size : new_capacity;
            memcpy(new_array, array, copy_count * sizeof(Value));
            free(array);
        }

        array = new_array;
        array_capacity = new_capacity;
    }

    // ========================================================================
    // RESIZE HASH - Rehash with optimized insert
    // ========================================================================
    void resize_hash(size_t new_capacity)
    {
        HashNode *old_buckets = hash_buckets;
        size_t old_capacity = hash_capacity;

        hash_capacity = new_capacity;
        hash_buckets = (HashNode *)calloc(hash_capacity, sizeof(HashNode));
        hash_size = 0;

        if (old_buckets)
        {
            for (size_t i = 0; i < old_capacity; ++i)
            {
                if (old_buckets[i].occupied)
                {

                    define(old_buckets[i].key, old_buckets[i].value);
                }
            }
            free(old_buckets);
        }
    }

    // ========================================================================
    // INT TO STRING - Manual itoa (10x faster than snprintf)
    // ========================================================================
    static const char *int_to_string(size_t num, char *buf_end)
    {
        *--buf_end = '\0';
        do
        {
            *--buf_end = '0' + (num % 10);
            num /= 10;
        } while (num > 0);
        return buf_end;
    }

public:
    // ========================================================================
    // CONSTRUCTION
    // ========================================================================
    Table()
        : array(nullptr), array_size(0), array_capacity(0),
          hash_buckets(nullptr), hash_size(0), hash_capacity(0)

    {
    }

    ~Table()
    {
        clear();
    
    if (array) 
        free(array);
    
    if (hash_buckets) 
        free(hash_buckets);
    }

    // GET_INDEX: Retorna índice interno ou -1 se não existir
    // Uso: compiler pode cachear índices
    inline int get_index(const char *key) const
    {
        if (hash_capacity == 0)
            return -1;

        size_t h = hash_string(key);
        size_t slot = h & (hash_capacity - 1);
        size_t original_slot = slot;

        do
        {
            const HashNode &bucket = hash_buckets[slot];

            if (!bucket.occupied)
                return -1;

            if (strcmp(bucket.key, key) == 0)
                return (int)slot; // Retorna slot como índice!

            slot = (slot + 1) & (hash_capacity - 1);
        } while (slot != original_slot);

        return -1;
    }

    // GET_BY_INDEX: Acesso direto por slot (ultra rápido!)
    inline Value *get_by_index(int slot)
    {
        if (slot < 0 || (size_t)slot >= hash_capacity)
            return nullptr;

        if (!hash_buckets[slot].occupied)
            return nullptr;

        return &hash_buckets[slot].value; // Direto!
    }

    // SET_BY_INDEX: Atualiza direto por slot
    inline bool set_by_index(int slot, const Value &value)
    {
        if (slot < 0 || (size_t)slot >= hash_capacity)
            return false;

        if (!hash_buckets[slot].occupied)
            return false;

        hash_buckets[slot].value = value;
        return true;
    }

    // ========================================================================
    // CLEAR
    // ========================================================================
    void clear()
    {
        if (array)
        {
            for (size_t i = 0; i < array_size; ++i)
                array[i].type = VAL_NULL;
            array_size = 0;
        }

 
        if (hash_buckets)
        {
            
            for (size_t i = 0; i < hash_capacity; ++i)
            {
                hash_buckets[i].occupied = false;
                hash_buckets[i].key[0] = '\0';
            }
            hash_size = 0;
        }
    }

    // ========================================================================
    // STATS
    // ========================================================================
    size_t size() const
    {
        size_t count = 0;
        for (size_t i = 0; i < array_size; ++i)
            if (array[i].type != VAL_NULL)
                ++count;
        return count + hash_size;
    }

    size_t array_count() const
    {
        size_t count = 0;
        for (size_t i = 0; i < array_size; ++i)
            if (array[i].type != VAL_NULL)
                ++count;
        return count;
    }

    size_t hash_count() const { return hash_size; }
    bool empty() const { return array_count() == 0 && hash_size == 0; }

    // ========================================================================
    // ITERATION - Methods added back for testing
    // ========================================================================
    template <typename Func>
    void for_each_array(Func func) const
    {
        for (size_t i = 0; i < array_size; ++i)
            if (array[i].type != VAL_NULL)
                func(i, array[i]);
    }

    template <typename Func>
    void for_each_hash(Func func) const
    {
        if (!hash_buckets)
            return;

        for (size_t i = 0; i < hash_capacity; ++i)
            if (hash_buckets[i].occupied)
                func(hash_buckets[i].key, hash_buckets[i].value);
    }

    // DEFINE: Insere apenas se NÃO existir. Retorna true se definiu, false se já existia
    // NÃO atualiza o valor se já existir - é para definição de variáveis novas

    inline bool define(const char *key_str, const Value &value)
    {
        if (hash_capacity == 0)
            resize_hash(INITIAL_HASH_CAPACITY);

        if ((hash_size + 1) * 100 > hash_capacity * MAX_LOAD_PERCENT)
            resize_hash(hash_capacity * 2);

        size_t h = hash_string(key_str);
        size_t slot = h & (hash_capacity - 1);
        size_t original_slot = slot;

        do
        {
            HashNode &bucket = hash_buckets[slot];

            if (!bucket.occupied)
            {

                bucket.set_key(key_str);
                bucket.value = value;
                bucket.occupied = true;
                ++hash_size;
                return true;
            }

            // ✅ Usa comparação de buffer
            if (bucket.key_equals(key_str))
            {
                return false; // Já existia
            }

            slot = (slot + 1) & (hash_capacity - 1);
        } while (slot != original_slot);

        return false;
    }

    // GET_PTR: Retorna ponteiro direto para Value, nullptr se não existir
    // Evita cópia de Value. Uso: OP_GET_GLOBAL
    inline Value *get_ptr(const char *key_str)
    {
        if (hash_capacity == 0)
            return nullptr;

        size_t h = hash_string(key_str);
        size_t slot = h & (hash_capacity - 1);
        size_t original_slot = slot;

        do
        {
            HashNode &bucket = hash_buckets[slot];

            if (!bucket.occupied)
                return nullptr;

            if (bucket.key_equals(key_str))
                return &bucket.value;

            slot = (slot + 1) & (hash_capacity - 1);
        } while (slot != original_slot);

        return nullptr;
    }

    // SET_IF_EXISTS: Atualiza apenas se existir. Retorna true se atualizou, false se não existia
    // Uso: OP_SET_GLOBAL
    inline bool set_if_exists(const char *key_str, const Value &value)
    {
        if (hash_capacity == 0)
            return false;

        size_t h = hash_string(key_str);
        size_t slot = h & (hash_capacity - 1);
        size_t original_slot = slot;

        do
        {
            HashNode &bucket = hash_buckets[slot];

            if (!bucket.occupied)
                return false;

            if (bucket.key_equals(key_str))
            {
                bucket.value = value;
                return true;
            }

            slot = (slot + 1) & (hash_capacity - 1);
        } while (slot != original_slot);

        return false;
    }
};
