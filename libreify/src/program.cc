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

#include "reify/program.hh"
#include "gringo/symbol.hh"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

namespace Reify {

// {{{1 Reifier

Reifier::Reifier(std::ostream &out, bool calculateSCCs, bool reifyStep)
    : out_(out), calculateSCCs_(calculateSCCs), reifyStep_(reifyStep) {}

Reifier::~Reifier() noexcept = default;

template <typename... T> void Reifier::printFact(char const *name, T const &...args) {
    out_ << name << "(";
    printComma(out_, args...);
    out_ << ").\n";
}
template <typename... T> void Reifier::printStepFact(char const *name, T const &...args) {
    if (reifyStep_) {
        printFact(name, args..., step_);
    } else {
        printFact(name, args...);
    }
}

template <class M, class T> size_t Reifier::tuple(M &map, char const *name, T const &args) {
    return tuple(map, name, toVec(args));
}

template <class M, class T> size_t Reifier::tuple(M &map, char const *name, std::vector<T> &&args) {
    auto ret = map.emplace(std::move(args), map.size());
    if (ret.second) {
        printStepFact(name, ret.first->second);
        for (auto &x : ret.first->first) {
            printStepFact(name, ret.first->second, x);
        }
    }
    return ret.first->second;
}

template <class M, class T> size_t Reifier::ordered_tuple(M &map, char const *name, T const &args) {
    return ordered_tuple(map, name, toVec(args));
}

template <class M, class T> size_t Reifier::ordered_tuple(M &map, char const *name, std::vector<T> &&args) {
    auto ret = map.emplace(std::move(args), map.size());
    if (ret.second) {
        printStepFact(name, ret.first->second);
        int arg = 0;
        for (auto &x : ret.first->first) {
            printStepFact(name, ret.first->second, arg, x);
            ++arg;
        }
    }
    return ret.first->second;
}

size_t Reifier::theoryTuple(IdSpan args) { return ordered_tuple(stepData_.theoryTuples, "theory_tuple", args); }

size_t Reifier::theoryElementTuple(IdSpan args) {
    return tuple(stepData_.theoryElementTuples, "theory_element_tuple", args);
}

size_t Reifier::litTuple(LitSpan args) { return tuple(stepData_.litTuples, "literal_tuple", args); }

size_t Reifier::weightLitTuple(WeightLitSpan args) {
    WLVec lits;
    lits.reserve(args.size());
    for (auto &x : args) {
        lits.emplace_back(x.lit, x.weight);
    }
    return tuple(stepData_.weightLitTuples, "weighted_literal_tuple", std::move(lits));
}

size_t Reifier::atomTuple(AtomSpan args) { return tuple(stepData_.atomTuples, "atom_tuple", args); }

Reifier::Graph::Node &Reifier::addNode(Atom_t atom) {
    auto &node = stepData_.nodes_[atom];
    if (!node) {
        node = &stepData_.graph_.insertNode(atom);
    }
    return *node;
}

void Reifier::initProgram(bool incremental) {
    if (incremental) {
        printFact("tag", "incremental");
    }
}

void Reifier::beginStep() {}

void Reifier::rule(HeadType ht, AtomSpan head, LitSpan body) {
    char const *h = ht == Potassco::HeadType::disjunctive ? "disjunction" : "choice";
    std::ostringstream hss, bss;
    hss << h << "(" << atomTuple(head) << ")";
    bss << "normal(" << litTuple(body) << ")";
    printStepFact("rule", hss.str(), bss.str());
    if (calculateSCCs_) {
        calculateSCCs(head, body);
    }
}

void Reifier::rule(HeadType ht, AtomSpan head, Weight_t bound, WeightLitSpan body) {
    char const *h = ht == Potassco::HeadType::disjunctive ? "disjunction" : "choice";
    std::ostringstream hss, bss;
    hss << h << "(" << atomTuple(head) << ")";
    bss << "sum(" << weightLitTuple(body) << "," << bound << ")";
    printStepFact("rule", hss.str(), bss.str());
    if (calculateSCCs_) {
        calculateSCCs(head, body);
    }
}

template <class L> void Reifier::calculateSCCs(AtomSpan head, std::span<const L> body) {
    for (auto &atom : head) {
        Graph::Node &u = addNode(atom);
        for (auto &elem : body) {
            if (Potassco::lit(elem) > 0) {
                Graph::Node &v = addNode(Potassco::lit(elem));
                u.insertEdge(v);
            }
        }
    }
}

void Reifier::minimize(Weight_t prio, WeightLitSpan lits) {
    printStepFact("minimize", prio, weightLitTuple(lits));
}

void Reifier::project(AtomSpan atoms) {
    for (auto &x : atoms) {
        printStepFact("project", x);
    }
}

void Reifier::outputAtom(Atom_t atom, std::string_view name) {
    printStepFact("outputAtom", name, atom);
}

void Reifier::outputTerm(Id_t termId, std::string_view name) {
    printStepFact("outputTerm", name, termId);
}

void Reifier::output(Id_t termId, LitSpan condition) {
    printStepFact("output", termId, litTuple(condition));
}

void Reifier::external(Atom_t a, TruthValue v) {
    char const *type = "";
    switch (v) {
        case TruthValue::free: {
            type = "free";
            break;
        }
        case TruthValue::false_: {
            type = "false";
            break;
        }
        case TruthValue::true_: {
            type = "true";
            break;
        }
        case TruthValue::release: {
            type = "release";
            break;
        }
    }
    printStepFact("external", a, type);
}

void Reifier::assume(LitSpan lits) {
    for (auto &x : lits) {
        printStepFact("assume", x);
    }
}

void Reifier::heuristic(Atom_t a, DomModifier t, int bias, unsigned prio, LitSpan condition) {
    char const *type = "";
    switch (t) {
        case DomModifier::level: {
            type = "level";
            break;
        }
        case DomModifier::sign: {
            type = "sign";
            break;
        }
        case DomModifier::factor: {
            type = "factor";
            break;
        }
        case DomModifier::init: {
            type = "init";
            break;
        }
        case DomModifier::true_: {
            type = "true";
            break;
        }
        case DomModifier::false_: {
            type = "false";
            break;
        }
    }
    printStepFact("heuristic", a, type, bias, prio, litTuple(condition));
}

void Reifier::acycEdge(int s, int t, LitSpan condition) { printStepFact("edge", s, t, litTuple(condition)); }

void Reifier::theoryTerm(Id_t termId, int number) { printStepFact("theory_number", termId, number); }

void Reifier::theoryTerm(Id_t termId, std::string_view name) {
    auto s = Gringo::quote(name);
    s.insert(s.begin(), '"');
    s.push_back('"');
    printStepFact("theory_string", termId, s);
}

void Reifier::theoryTerm(Id_t termId, int cId, IdSpan args) {
    if (cId >= 0) {
        printStepFact("theory_function", termId, cId, theoryTuple(args));
    } else {
        char const *type = "";
        switch (cId) {
            case -1: {
                type = "tuple";
                break;
            }
            case -2: {
                type = "set";
                break;
            }
            case -3: {
                type = "list";
                break;
            }
        }
        printStepFact("theory_sequence", termId, type, theoryTuple(args));
    }
}

void Reifier::theoryElement(Id_t elementId, IdSpan terms, LitSpan cond) {
    auto tt = theoryTuple(terms);
    auto lt = litTuple(cond);
    printStepFact("theory_element", elementId, tt, lt);
}

void Reifier::theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan elements) {
    printStepFact("theory_atom", atomOrZero, termId, theoryElementTuple(elements));
}

void Reifier::theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan elements, Id_t op, Id_t rhs) {
    printStepFact("theory_atom", atomOrZero, termId, theoryElementTuple(elements), op, rhs);
}

void Reifier::endStep() {
    size_t i = 0;
    for (auto &scc : stepData_.graph_.tarjan()) {
        if (scc.size() > 1) {
            for (auto &node : scc) {
                printStepFact("scc", i, node->data);
            }
        }
        ++i;
    }
    if (reifyStep_) {
        stepData_ = StepData();
        ++step_;
    }
}

void Reifier::parse(std::istream &in) { Potassco::readAspif(in, *this); }

} // namespace Reify
