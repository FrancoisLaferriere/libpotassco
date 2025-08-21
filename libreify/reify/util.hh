// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}


#pragma once

#include <potassco/basic_types.h>

#include <iostream>
#include <vector>

namespace Reify {

template <typename T>
void printValue(std::ostream& out, const T& value) { out << value; }

template <typename T, typename U>
void printValue(std::ostream& out, const std::pair<T, U>& value) {
    printValue(out, value.first);
    out << ",";
    printValue(out, value.second);
}

template <typename T, typename... V>
void printComma(std::ostream& out, const T& t, const V& ...v) {
    printValue(out, t);
    ((out << "," , printValue(out, v)), ...);
}

template <typename T>
struct Hash : std::hash<T> {};

template <typename T, typename U>
struct Hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U>& p) const noexcept {
        size_t hash = std::hash<T>()(p.first);
        hash ^= Hash<U>()(p.second) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

template <typename T>
struct Hash<std::vector<T>> {
    size_t operator()(const std::vector<T>& vec) const noexcept {
        size_t hash = vec.size();
        for (auto& x : vec) {
            hash ^= Hash<typename std::vector<T>::value_type>()(x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

template <typename T>
std::vector<T> toVec(std::span<const T> span) { return {span.begin(), span.end()}; }

inline std::string quote(std::string_view str) {
    std::string res;
    res.push_back('"');
    for (auto c : str) {
        switch (c) {
            case '\n': {
                res.push_back('\\');
                res.push_back('n');
                break;
            }
            case '\\': {
                res.push_back('\\');
                res.push_back('\\');
                break;
            }
            case '"': {
                res.push_back('\\');
                res.push_back('"');
                break;
            }
            default: {
                res.push_back(c);
                break;
            }
        }
    }
    res.push_back('"');
    return res;
}

} // namespace Reify
