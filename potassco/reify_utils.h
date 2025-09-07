//
// Copyright (c) 2017 - 2025, Roland Kaminski
// Copyright (c) 2025 - present, Francois Laferriere
//
// This file is part of Potassco.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
#pragma once

#include <potassco/basic_types.h>

#include <ostream>
#include <vector>

namespace Potassco {

struct Head {
    HeadType type;
    size_t   id;
};
struct Normal {
    size_t id;
};
struct Sum {
    size_t   id;
    Weight_t bound;
};
struct Quoted {
    std::string_view str;
};

template <typename T>
void printValue(std::ostream& out, const T& value) {
    out << value;
}

inline void printValue(std::ostream& out, const WeightLit& value) {
    printValue(out, value.lit);
    out << ",";
    printValue(out, value.weight);
}

inline void printValue(std::ostream& out, const Head& h) {
    const char* name = (h.type == HeadType::disjunctive ? "disjunction" : "choice");
    out << name << "(" << h.id << ")";
}

inline void printValue(std::ostream& out, const Normal& n) { out << "normal(" << n.id << ")"; }

inline void printValue(std::ostream& out, const Sum& s) { out << "sum(" << s.id << "," << s.bound << ")"; }

inline void printValue(std::ostream& out, const Quoted& q) {
    out.put('"');
    for (auto c : q.str) {
        switch (c) {
            case '\n': out << "\\n"; break;
            case '\\': out << "\\\\"; break;
            case '"' : out << "\\\""; break;
            default  : out.put(c); break;
        }
    }
    out.put('"');
}

template <typename T, typename... V>
void printCommaSeparated(std::ostream& out, const T& t, const V&... v) {
    printValue(out, t);
    ((out << ",", printValue(out, v)), ...);
}
template <typename T>
struct VectorHash {
    size_t operator()(const std::vector<T>& vec) const {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        auto* data = reinterpret_cast<const char*>(vec.data());
        return std::hash<std::string_view>{}({data, vec.size() * sizeof(T)});
    }
};

template <typename T>
std::vector<T> toVec(std::span<const T> span) {
    return {span.begin(), span.end()};
}

} // namespace Potassco
