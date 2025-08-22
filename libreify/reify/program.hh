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

#include <cstdint>
#include <gringo/graph.hh>
#include <potassco/aspif.h>
#include <reify/util.hh>

#include <unordered_map>
#include <vector>

namespace Reify {

using Potassco::Atom_t;
using Potassco::AtomSpan;
using Potassco::HeadType;
using Potassco::DomModifier;
using Potassco::Id_t;
using Potassco::IdSpan;
using Potassco::Lit_t;
using Potassco::LitSpan;
using Potassco::TruthValue;
using Potassco::Weight_t;
using Potassco::WeightLit;
using Potassco::WeightLitSpan;

class Reifier : public Potassco::AbstractProgram {
public:
    explicit Reifier(std::ostream& out, bool calculateSCCs, bool reifyStep);
    ~Reifier() noexcept override;
    Reifier(const Reifier&)            = delete;
    Reifier& operator=(const Reifier&) = delete;

    void parse(std::istream& in);

    void initProgram(bool incremental) override;
    void beginStep() override;
    void rule(HeadType ht, AtomSpan head, LitSpan body) override;
    void rule(HeadType ht, AtomSpan head, Weight_t bound, WeightLitSpan body) override;
    void minimize(Weight_t prio, WeightLitSpan lits) override;
    void project(AtomSpan atoms) override;
    void outputAtom(Atom_t atom, std::string_view name) override;
    void outputTerm(Id_t termId, std::string_view name) override;
    void output(Id_t termId, LitSpan condition) override;
    void external(Atom_t a, TruthValue v) override;
    void assume(LitSpan lits) override;
    void heuristic(Atom_t a, DomModifier t, int bias, unsigned prio, LitSpan condition) override;
    void acycEdge(int s, int t, LitSpan condition) override;

    void theoryTerm(Id_t termId, int number) override;
    void theoryTerm(Id_t termId, std::string_view name) override;
    void theoryTerm(Id_t termId, int cId, IdSpan args) override;
    void theoryElement(Id_t elementId, IdSpan terms, LitSpan cond) override;
    void theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan elements) override;
    void theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan elements, Id_t op, Id_t rhs) override;

    void endStep() override;

private:
    using Graph = Gringo::Graph<Atom_t>;
    template <typename L>
    void calculateSCCs(AtomSpan head, std::span<const L> body);

    template <typename... T>
    void printFact(const char* name, const T&... args);
    template <typename... T>
    void printStepFact(const char* name, const T&... args);
    template <typename M, typename T>
    auto tuple(M& map, const char* name, const T& args) -> size_t;
    template <typename M, typename T>
    auto tuple(M& map, const char* name, std::vector<T>&& args) -> size_t;
    template <typename M, typename T>
    auto orderedTuple(M& map, const char* name, const T& args) -> size_t;
    template <typename M, typename T>
    auto orderedTuple(M& map, const char* name, std::vector<T>&& args) -> size_t;
    auto theoryTuple(IdSpan args) -> size_t;
    auto litTuple(LitSpan args) -> size_t;
    auto atomTuple(AtomSpan args) -> size_t;
    auto theoryElementTuple(IdSpan args) -> size_t;
    auto weightLitTuple(WeightLitSpan args) -> size_t;
    auto addNode(Atom_t atom) -> Graph::Node&;

private:
    using WLVec = std::vector<std::pair<Lit_t, Weight_t>>;
    struct StepData {
        std::unordered_map<std::vector<Id_t>, size_t, Hash<std::vector<Id_t>>> theoryTuples;
        std::unordered_map<std::vector<Id_t>, size_t, Hash<std::vector<Id_t>>> theoryElementTuples;
        std::unordered_map<std::vector<Lit_t>, size_t, Hash<std::vector<Lit_t>>> litTuples;
        std::unordered_map<std::vector<Atom_t>, size_t, Hash<std::vector<Atom_t>>> atomTuples;
        std::unordered_map<WLVec, size_t, Hash<WLVec>> weightLitTuples;
        Graph graph_;
        std::unordered_map<Atom_t, Graph::Node*> nodes_;
    } stepData_;
    std::ostream& out_;
    size_t step_ = 0;
    bool calculateSCCs_;
    bool reifyStep_;
};

} // namespace Reify
