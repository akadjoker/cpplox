#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class String
{
public:
    // SSO: strings até 23 bytes ficam inline (24 bytes total)
    static constexpr size_t SSO_CAPACITY = 23;

    String() : size_(0), isHeap_(false)
    {
        data_.buffer[0] = '\0';
    }

    String(const char *str)
    {
        size_t len = std::strlen(str);
        size_ = len;

        if (len <= SSO_CAPACITY)
        {
            // Small: inline storage
            isHeap_ = false;
            std::memcpy(data_.buffer, str, len);
            data_.buffer[len] = '\0';
        }
        else
        {
            // Large: heap storage
            isHeap_ = true;
            data_.ptr = new char[len + 1];
            std::memcpy(data_.ptr, str, len);
            data_.ptr[len] = '\0';
        }
    }

    String(const String &other)
    {
        size_ = other.size_;
        isHeap_ = other.isHeap_;

        if (isHeap_)
        {
            data_.ptr = new char[size_ + 1];
            std::memcpy(data_.ptr, other.data_.ptr, size_ + 1);
        }
        else
        {
            std::memcpy(data_.buffer, other.data_.buffer, size_ + 1);
        }
    }

    String(String &&other) noexcept
    {
        size_ = other.size_;
        isHeap_ = other.isHeap_;

        if (isHeap_)
        {
            data_.ptr = other.data_.ptr;
            other.data_.ptr = nullptr;
            other.size_ = 0;
        }
        else
        {
            std::memcpy(data_.buffer, other.data_.buffer, size_ + 1);
        }
    }

    ~String()
    {
        if (isHeap_)
        {
            delete[] data_.ptr;
        }
    }

    String &operator=(const String &other)
    {
        if (this != &other)
        {
            if (isHeap_)
                delete[] data_.ptr;

            size_ = other.size_;
            isHeap_ = other.isHeap_;

            if (isHeap_)
            {
                data_.ptr = new char[size_ + 1];
                std::memcpy(data_.ptr, other.data_.ptr, size_ + 1);
            }
            else
            {
                std::memcpy(data_.buffer, other.data_.buffer, size_ + 1);
            }
        }
        return *this;
    }

    // Getters
    const char *c_str() const
    {
        return isHeap_ ? data_.ptr : data_.buffer;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    bool isHeap() const { return isHeap_; }

    // Comparison
    bool operator==(const String &other) const
    {
        if (size_ != other.size_)
            return false;
        return std::strcmp(c_str(), other.c_str()) == 0;
    }

    bool operator!=(const String &other) const
    {
        return !(*this == other);
    }

    // Concatenation
    String operator+(const String &other) const
    {
        size_t newSize = size_ + other.size_;
        char *temp = new char[newSize + 1];
        std::memcpy(temp, c_str(), size_);
        std::memcpy(temp + size_, other.c_str(), other.size_);
        temp[newSize] = '\0';

        String result(temp);
        delete[] temp;
        return result;
    }

private:
    union
    {
        char *ptr;       // Heap pointer (large strings)
        char buffer[24]; // Inline buffer (small strings)
    } data_;

    uint32_t size_; // String length
    bool isHeap_;   // Heap flag
};
