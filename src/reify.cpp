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

#include <potassco/enum.h>
#include <potassco/reify.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

namespace Potassco {

/////////////////////////////////////////////////////////////////////////////////////////
// Reifier
/////////////////////////////////////////////////////////////////////////////////////////
Reifier::Reifier(std::ostream& out, const Options& opts)
    : out_(out), calculateSCCs_(opts.calculateSCCs), reifyStep_(opts.reifyStep) {}

Reifier::~Reifier() noexcept = default;

template <typename... T>
void Reifier::printFact(const char* name, const T&... args) {
    out_ << name << "(";
    printComma(out_, args...);
    out_ << ").\n";
}

template <typename... T>
void Reifier::printStepFact(const char* name, const T&... args) {
    if (reifyStep_) {
        printFact(name, args..., step_);
    } else {
        printFact(name, args...);
    }
}

template <typename M, typename T>
auto Reifier::tuple(M& map, const char* name, const T& args) -> size_t {
    return tuple(map, name, toVec(args));
}

template <typename M, typename T>
auto Reifier::tuple(M& map, const char* name, std::vector<T>&& args) -> size_t {
    auto ret = map.emplace(std::move(args), map.size());
    if (ret.second) {
        printStepFact(name, ret.first->second);
        for (const auto& x : ret.first->first) {
            printStepFact(name, ret.first->second, x);
        }
    }
    return ret.first->second;
}

template <typename M, typename T>
auto Reifier::orderedTuple(M& map, const char* name, const T& args) -> size_t {
    return orderedTuple(map, name, toVec(args));
}

template <typename M, typename T>
auto Reifier::orderedTuple(M& map, const char* name, std::vector<T>&& args) -> size_t {
    auto ret = map.emplace(std::move(args), map.size());
    if (ret.second) {
        printStepFact(name, ret.first->second);
        int arg = 0;
        for (const auto& x : ret.first->first) {
            printStepFact(name, ret.first->second, arg, x);
            ++arg;
        }
    }
    return ret.first->second;
}

auto Reifier::theoryTuple(IdSpan args) -> size_t {
    return orderedTuple(stepData_.theoryTuples, "theory_tuple", args);
}

auto Reifier::theoryElementTuple(IdSpan args) -> size_t {
    return tuple(stepData_.theoryElementTuples, "theory_element_tuple", args);
}

auto Reifier::litTuple(LitSpan args) -> size_t {
    return tuple(stepData_.litTuples, "literal_tuple", args);
}

auto Reifier::weightLitTuple(WeightLitSpan args) -> size_t {
    WLVec lits;
    lits.reserve(args.size());
    for (const auto& x : args) {
        lits.emplace_back(x.lit, x.weight);
    }
    return tuple(stepData_.weightLitTuples, "weighted_literal_tuple", std::move(lits));
}

auto Reifier::atomTuple(AtomSpan args) -> size_t {
    return tuple(stepData_.atomTuples, "atom_tuple", args);
}

auto Reifier::addNode(Atom_t atom) -> uint32_t {
    auto& nodesMap = stepData_.nodes_;
    auto it = nodesMap.find(atom);
    if (it != nodesMap.end()) {
        return it->second;
    }
    auto nodeId = stepData_.graph_.addNode(atom);
    nodesMap[atom] = nodeId;
    return nodeId;
}

void Reifier::initProgram(bool incremental) {
    if (incremental) {
        printFact("tag", "incremental");
    }
}

void Reifier::beginStep() {}

void Reifier::rule(HeadType ht, AtomSpan head, LitSpan body) {
    auto headId = atomTuple(head);
    auto bodyId = litTuple(body);
    printStepFact("rule", Head{ht, headId}, Normal{bodyId});
    if (calculateSCCs_) {
        calculateSCCs(head, body);
    }
}

void Reifier::rule(HeadType ht, AtomSpan head, Weight_t bound, WeightLitSpan body) {
    auto headId = atomTuple(head);
    auto bodyId = weightLitTuple(body);
    printStepFact("rule", Head{ht, headId}, Sum{bodyId, bound});
    if (calculateSCCs_) {
        calculateSCCs(head, body);
    }
}

template <typename L>
void Reifier::calculateSCCs(AtomSpan head, std::span<const L> body) {
    for (const auto& atom : head) {
        auto uId = addNode(atom);
        for (const auto& elem : body) {
            if (lit(elem) > 0) {
                auto vId = addNode(static_cast<Atom_t>(lit(elem)));
                stepData_.graph_.addEdge(uId, vId);
            }
        }
    }
}

void Reifier::minimize(Weight_t prio, WeightLitSpan lits) {
    printStepFact("minimize", prio, weightLitTuple(lits));
}

void Reifier::project(AtomSpan atoms) {
    for (const auto& x : atoms) {
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
    printStepFact("external", a, enum_name(v));
}

void Reifier::assume(LitSpan lits) {
    for (const auto& x : lits) {
        printStepFact("assume", x);
    }
}

void Reifier::heuristic(Atom_t a, DomModifier t, int bias, unsigned prio, LitSpan condition) {
    printStepFact("heuristic", a, enum_name(t), bias, prio, litTuple(condition));
}

void Reifier::acycEdge(int s, int t, LitSpan condition) {
    printStepFact("edge", s, t, litTuple(condition));
}

void Reifier::theoryTerm(Id_t termId, int number) {
    printStepFact("theory_number", termId, number);
}

void Reifier::theoryTerm(Id_t termId, std::string_view name) {
    printStepFact("theory_string", termId, Quoted{name});
}

void Reifier::theoryTerm(Id_t termId, int cId, IdSpan args) {
    if (cId >= 0) {
        printStepFact("theory_function", termId, cId, theoryTuple(args));
    } else {
        const char* type = "";
        switch (cId) {
            case -1: type = "tuple"; break;
            case -2: type = "set";   break;
            case -3: type = "list";  break;
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
    for (const auto& scc : stepData_.graph_.computeNonTrivialSccs()) {
        for (auto it = scc.rbegin(); it != scc.rend(); ++it) {
            printStepFact("scc", i, *it);
        }
        ++i;
    }
    if (reifyStep_) {
        stepData_ = StepData();
        ++step_;
    }
}

void Reifier::parse(std::istream& in) { readAspif(in, *this); }

} // namespace Potassco
