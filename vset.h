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
#include <vector>

// minimal set and map implementations based on std::vector. Faster due
// to cache locality when the number of elements is small.
template <typename T>
struct vset : std::vector<T> {

    void add(T v)
    {
        if (std::find(this->begin(), this->end(), v) == this->end())
            this->push_back(v);
    }

    void remove(T v)
    {
        auto it = std::find(this->begin(), this->end(), v);
        if (it != this->end()) {
            unsigned int pos = it - this->begin();
            if (pos < this->size() - 1)
                this->at(pos) = this->at(this->size() - 1);
            this->pop_back();
        }
    }
};

template <typename T, typename U>
struct vmap : std::vector<std::pair<T, U>> {

    auto find(T v)
    {
        auto it = this->begin();
        for (; it != this->end(); it++)
            if ((*it).first == v)
                break;
        return it;
    }

    U &add(T v, U data)
    {
        auto it = find(v);
        if (it == this->end())
            it = this->insert(it, {v, data});
        return (*it).second;
    }

    void remove(T v)
    {
        auto it = find(v);
        if (it != this->end()) {
            unsigned int pos = it - this->begin();
            if (pos < this->size() - 1)
                this->at(pos) = this->at(this->size() - 1);
            this->pop_back();
        }
    }
};
