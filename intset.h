/*
 * Copyright (c) 2013-2019 Emanuele Giaquinta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>

class intset {
private:
    using block = unsigned long;
    static const unsigned bits_per_block = std::numeric_limits<block>::digits;
    static unsigned block_index(unsigned n) { return n / bits_per_block; }
    static unsigned bit_index(unsigned n) { return n % bits_per_block; }
    static block bit_mask(unsigned n) { return block(1) << bit_index(n); }

    std::unique_ptr<block[]> data_;
    unsigned num_bits_;

    unsigned num_blocks() const
    {
        return (num_bits_ + bits_per_block - 1) / bits_per_block;
    }

public:
    unsigned max_size() const { return num_bits_; }

    intset(unsigned size)
    {
        num_bits_ = size;
        data_ = std::make_unique<block[]>(num_blocks());
    }

    intset(const intset &s)
    {
        num_bits_ = s.num_bits_;
        data_ = std::make_unique<block[]>(num_blocks());
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] = s.data_[i];
    }

    intset &operator=(const intset &s)
    {
        if (num_blocks() != s.num_blocks()) {
            auto data = std::make_unique<block[]>(s.num_blocks());
            data_ = std::move(data);
        }
        num_bits_ = s.num_bits_;
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] = s.data_[i];
        return *this;
    }

    intset(intset &&s) noexcept
    {
        data_.swap(s.data_);
        std::swap(num_bits_, s.num_bits_);
    }

    intset &operator=(intset &&s) noexcept
    {
        data_.swap(s.data_);
        std::swap(num_bits_, s.num_bits_);
        return *this;
    }

    bool operator==(const intset &s) const
    {
        auto n = std::min({num_blocks(), s.num_blocks()});
        for (unsigned i = 0; i < n; i++)
            if (data_[i] != s.data_[i])
                return false;
        for (unsigned i = n; i < num_blocks(); i++)
            if (data_[i])
                return false;
        for (unsigned i = n; i < s.num_blocks(); i++)
            if (s.data_[i])
                return false;
        return true;
    }

    intset &add(unsigned n)
    {
        assert(n < num_bits_);
        data_[block_index(n)] |= bit_mask(n);
        return *this;
    }

    intset &add(const intset &s)
    {
        assert(num_bits_ == s.num_bits_);
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] |= s.data_[i];
        return *this;
    }

    intset &remove(unsigned n)
    {
        assert(n < num_bits_);
        data_[block_index(n)] &= ~bit_mask(n);
        return *this;
    }

    intset &remove(const intset &s)
    {
        assert(num_bits_ == s.num_bits_);
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] &= ~s.data_[i];
        return *this;
    }

    intset &intersect(const intset &s)
    {
        assert(num_bits_ == s.num_bits_);
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] &= s.data_[i];
        return *this;
    }

    void clear()
    {
        for (unsigned i = 0; i < num_blocks(); i++)
            data_[i] = 0;
    }

    bool contains(unsigned n) const
    {
        return data_[block_index(n)] & bit_mask(n);
    }

    bool is_subset_of(const intset &s) const
    {
        auto n = std::min({num_blocks(), s.num_blocks()});
        for (unsigned i = 0; i < n; i++)
            if (data_[i] & ~s.data_[i])
                return false;
        for (unsigned i = n; i < num_blocks(); i++)
            if (data_[i])
                return false;
        return true;
    }

    bool intersects(const intset &s) const
    {
        auto n = std::min({num_blocks(), s.num_blocks()});
        for (unsigned i = 0; i < n; i++)
            if (data_[i] & s.data_[i])
                return true;
        return false;
    }

    unsigned minimum() const
    {
        for (unsigned i = 0; i < num_blocks(); i++)
            if (data_[i])
                return __builtin_ctzl(data_[i]) + i * bits_per_block;
        return -1;
    }

    unsigned size() const
    {
        unsigned size = 0;

        for (unsigned i = 0; i < num_blocks(); i++)
            if (data_[i])
                size += __builtin_popcountl(data_[i]);
        return size;
    }

    unsigned find_next(unsigned elem) const
    {
        if (elem >= num_bits_)
            return -1;
        unsigned long x = data_[block_index(elem)];
        int r = bit_index(elem);
        x &= block(~0) << r;
        elem -= r;
        while (!x) {
            elem += bits_per_block;
            if (elem >= num_bits_)
                return -1;
            x = data_[block_index(elem)];
        }
        elem += __builtin_ctzl(x);
        if (elem >= num_bits_)
            return -1;
        return elem;
    }

    // equivalent to .intersects(lhs & rhs)
    bool intersects(const intset &lhs, const intset &rhs) const
    {
        auto n = std::min({
            num_blocks(),
            lhs.num_blocks(),
            rhs.num_blocks(),
        });
        for (unsigned i = 0; i < n; i++)
            if (data_[i] & lhs.data_[i] & rhs.data_[i])
                return true;
        return false;
    }

    // equivalent to .intersects(lhs | rhs)
    bool intersects_union(const intset &lhs, const intset &rhs) const
    {
        auto n = std::min({
            num_blocks(),
            lhs.num_blocks(),
            rhs.num_blocks(),
        });
        for (unsigned i = 0; i < n; i++)
            if (data_[i] & (lhs.data_[i] | rhs.data_[i]))
                return true;
        return false;
    }

    // equivalent to .intersects(lhs - rhs)
    bool intersects_difference(const intset &lhs, const intset &rhs) const
    {
        auto n = std::min({
            num_blocks(),
            lhs.num_blocks(),
            rhs.num_blocks(),
        });
        for (unsigned i = 0; i < n; i++)
            if (data_[i] & (lhs.data_[i] & ~rhs.data_[i]))
                return true;
        return false;
    }

    class iterator {
        const intset &s_;
        unsigned elem_;

    public:
        iterator(const intset &s, unsigned elem)
            : s_(s)
        {
            elem_ = s_.find_next(elem);
        }
        iterator &operator++()
        {
            elem_ = s_.find_next(elem_ + 1);
            return *this;
        }
        iterator operator++(int)
        {
            iterator it = *this;
            ++(*this);
            return it;
        }
        bool operator==(iterator other) const { return elem_ == other.elem_; }
        bool operator!=(iterator other) const { return !(*this == other); }
        int operator*() { return elem_; }

        using difference_type = long long;
        using value_type = unsigned;
        using pointer = const unsigned *;
        using reference = const unsigned &;
        using iterator_category = std::input_iterator_tag;
    };
    iterator begin() const { return {*this, 0}; }
    iterator end() const { return {*this, num_bits_}; }
};

inline intset operator|(const intset &lhs, const intset &rhs)
{
    intset s(lhs);
    return s.add(rhs);
}

inline intset operator|(intset &&lhs, const intset &rhs)
{
    intset s(std::move(lhs));
    return s.add(rhs);
}

inline intset operator|(const intset &lhs, intset &&rhs)
{
    intset s(std::move(rhs));
    return s.add(lhs);
}

inline intset operator|(intset &&lhs, intset &&rhs)
{
    intset s(std::move(lhs));
    return s.add(rhs);
}

inline intset operator-(const intset &lhs, const intset &rhs)
{
    intset s(lhs);
    return s.remove(rhs);
}

inline intset operator-(intset &&lhs, const intset &rhs)
{
    intset s(std::move(lhs));
    return s.remove(rhs);
}

inline intset operator-(intset &&lhs, intset &&rhs)
{
    intset s(std::move(lhs));
    return s.remove(rhs);
}

inline intset operator&(const intset &lhs, const intset &rhs)
{
    intset s(lhs);
    return s.intersect(rhs);
}

inline intset operator&(intset &&lhs, const intset &rhs)
{
    intset s(std::move(lhs));
    return s.intersect(rhs);
}

inline intset operator&(const intset &lhs, intset &&rhs)
{
    intset s(std::move(rhs));
    return s.intersect(lhs);
}

inline intset operator&(intset &&lhs, intset &&rhs)
{
    intset s(std::move(lhs));
    return s.intersect(rhs);
}
