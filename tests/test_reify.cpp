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
#include <potassco/aspif_text.h>
#include <potassco/graph.h>
#include <potassco/reify.h>

#include <catch2/catch_test_macros.hpp>

#include <sstream>

namespace Potassco::Test::Reify {

static bool readText(std::stringstream& in, std::stringstream& out, bool scc = false, bool step = false) {
    Reifier        prg(out, {scc, step});
    AspifTextInput parser(&prg);
    return readProgram(in, parser) == 0;
}

static bool readAspif(std::stringstream& in, std::stringstream& out, bool scc = false, bool step = false) {
    Reifier    prg(out, {scc, step});
    AspifInput parser(prg);
    return readProgram(in, parser) == 0;
}

TEST_CASE("Test Reifier", "[reify]") {
    std::stringstream input, output;

    SECTION("empty") {
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "");
    }
    SECTION("incremental") {
        input << "#incremental.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "tag(incremental).\n");
    }
    SECTION("normal") {
        input << "a:-b.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "atom_tuple(0).\natom_tuple(0,1).\nliteral_tuple(0).\nliteral_tuple(0,2).\nrule("
                                "disjunction(0),normal(0)).\n");
    }
    SECTION("step") {
        input << "#incremental.";
        input << "a:-b.";
        input << "#step.";
        input << "c:-b.";
        auto result = "tag(incremental).\n"
                      "atom_tuple(0,0).\n"
                      "atom_tuple(0,1,0).\n"
                      "literal_tuple(0,0).\n"
                      "literal_tuple(0,2,0).\n"
                      "rule(disjunction(0),normal(0),0).\n"
                      "atom_tuple(0,1).\n"
                      "atom_tuple(0,3,1).\n"
                      "literal_tuple(0,1).\n"
                      "literal_tuple(0,2,1).\n"
                      "rule(disjunction(0),normal(0),1).\n";
        REQUIRE(readText(input, output, false, true));
        REQUIRE(output.str() == result);
    }
    SECTION("cycle") {
        input << "a:-b.";
        input << "b:-a.";
        auto result = "atom_tuple(0).\n"
                      "atom_tuple(0,1).\n"
                      "literal_tuple(0).\n"
                      "literal_tuple(0,2).\n"
                      "rule(disjunction(0),normal(0)).\n"
                      "atom_tuple(1).\n"
                      "atom_tuple(1,2).\n"
                      "literal_tuple(1).\n"
                      "literal_tuple(1,1).\n"
                      "rule(disjunction(1),normal(1)).\n"
                      "scc(0,1).\n"
                      "scc(0,2).\n";
        REQUIRE(readText(input, output, true));
        REQUIRE(output.str() == result);
    }
    SECTION("choice") {
        input << "{a, b}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() ==
                "atom_tuple(0).\natom_tuple(0,1).\natom_tuple(0,2).\nliteral_tuple(0).\nrule(choice(0),normal(0)).\n");
    }
    SECTION("sum") {
        input << ":-1 {a, b}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "atom_tuple(0).\nweighted_literal_tuple(0).\nweighted_literal_tuple(0,1,1).\nweighted_"
                                "literal_tuple(0,2,1).\nrule(disjunction(0),sum(0,1)).\n");
    }
    SECTION("minimize") {
        input << "#minimize {a=10, b=20}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "weighted_literal_tuple(0).\nweighted_literal_tuple(0,1,10).\nweighted_literal_tuple(0,"
                                "2,20).\nminimize(0,0).\n");
    }
    SECTION("project") {
        input << "#project {a}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "project(1).\n");
    }
    SECTION("output") {
        input << "#output a:b,c.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() ==
                "outputTerm(a,0).\nliteral_tuple(0).\nliteral_tuple(0,2).\nliteral_tuple(0,3).\noutput(0,0).\n");
    }
    SECTION("external") {
        input << "#external a.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "external(1,false).\n");
    }
    SECTION("assume") {
        input << "#assume {a}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "assume(1).\n");
    }
    SECTION("heuristic") {
        input << "#heuristic a. [1, level]";
        input << "#heuristic b : c. [2@1, true]";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "literal_tuple(0).\nheuristic(1,level,1,0,0).\nliteral_tuple(1).\nliteral_tuple(1,3)."
                                "\nheuristic(2,true,2,1,1).\n");
    }
    SECTION("edge") {
        input << "#edge (1,2) : a.";
        input << "#edge (2,1).";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() ==
                "literal_tuple(0).\nliteral_tuple(0,1).\nedge(1,2,0).\nliteral_tuple(1).\nedge(2,1,1).\n");
    }
    SECTION("unique tuples") {
        input << " {a; b}.";
        input << " {a; b}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "atom_tuple(0).\natom_tuple(0,1).\natom_tuple(0,2).\nliteral_tuple(0).\nrule(choice(0),"
                                "normal(0)).\nrule(choice(0),normal(0)).\n");
    }
    SECTION("sorted tuples") {
        input << " {a; b}.";
        input << " {b; a}.";
        REQUIRE(readText(input, output));
        REQUIRE(output.str() == "atom_tuple(0).\natom_tuple(0,1).\natom_tuple(0,2).\nliteral_tuple(0).\nrule(choice(0),"
                                "normal(0)).\nrule(choice(0),normal(0)).\n");
    }

    input << "asp 1 0 0\n";

    SECTION("theory terms") {
        input << "9 0 6 42\n";
        input << "9 1 0 6 banana\n";
        input << "9 2 14 4 2 12 13\n";
        input << "0";
        REQUIRE(readAspif(input, output));
        REQUIRE(output.str() == "theory_number(6,42).\ntheory_string(0,\"banana\").\ntheory_tuple(0).\ntheory_tuple(0,"
                                "0,12).\ntheory_tuple(0,1,13).\ntheory_function(14,4,0).\n");
    }
    SECTION("theory atoms") {
        input << "9 4 0 1 10 0\n";
        input << "9 5 6 0 1 1\n";
        input << "9 6 6 0 1 1 2 3\n";
        input << "0";
        REQUIRE(readAspif(input, output));
        REQUIRE(output.str() ==
                "theory_tuple(0).\ntheory_tuple(0,0,10).\nliteral_tuple(0).\ntheory_element(0,0,0).\ntheory_element_"
                "tuple(0).\ntheory_element_tuple(0,1).\ntheory_atom(6,0,0).\ntheory_atom(6,0,0,2,3).\n");
    }
    SECTION("theory unique tuples") {
        input << "9 2 14 4 2 12 13\n";
        input << "9 2 37 9 2 12 13\n";
        input << "0";
        REQUIRE(readAspif(input, output));
        REQUIRE(output.str() == "theory_tuple(0).\ntheory_tuple(0,0,12).\ntheory_tuple(0,1,13).\ntheory_function(14,4,"
                                "0).\ntheory_function(37,9,0).\n");
    }
    SECTION("theory quoting") {
        input << "9 1 0 6 hell\"o\n";
        input << "9 1 1 6 gre\\at\n";
        input << "9 1 2 6 worl\nd\n";
        input << "0";
        REQUIRE(readAspif(input, output));
        REQUIRE(output.str() ==
                "theory_string(0,\"hell\\\"o\").\ntheory_string(1,\"gre\\\\at\").\ntheory_string(2,\"worl\\nd\").\n");
    }
}

static std::string toString(const Graph<uint32_t>::SccVec& sccs) {
    std::ostringstream out;
    out << "[";
    std::string sccVecSeparator;
    for (const auto& scc : sccs) {
        out << sccVecSeparator << "[";
        std::string sccSeparator;
        for (auto id : scc) {
            out << sccSeparator << static_cast<char>('a' + id);
            sccSeparator = ",";
        }
        out << "]";
        sccVecSeparator = ",";
    }
    out << "]";
    return out.str();
}

TEST_CASE("Test Graph", "[reify]") {
    Graph<uint32_t> g;
    SECTION("empty graph") { REQUIRE(g.computeSccs().empty()); }
    SECTION("single node") {
        g.addNode(0);
        auto sccs = g.computeSccs();
        REQUIRE(sccs.size() == 1);
        REQUIRE(toString(sccs) == "[[a]]");
    }
    SECTION("acyclic graph") {
        auto idA = g.addNode(0); // a
        auto idB = g.addNode(1); // b
        auto idC = g.addNode(2); // c

        g.addEdge(idA, idB);
        g.addEdge(idB, idC);

        auto sccs = g.computeSccs();
        REQUIRE(sccs.size() == 3);
        REQUIRE(toString(sccs) == "[[c],[b],[a]]");

        REQUIRE(g.computeNonTrivialSccs().empty());
    }
    SECTION("single cycle") {
        auto idA = g.addNode(0);
        auto idB = g.addNode(1);
        auto idC = g.addNode(2);
        g.addNode(3);

        g.addEdge(idA, idB);
        g.addEdge(idB, idC);
        g.addEdge(idC, idA);

        auto sccs = g.computeSccs();
        REQUIRE(sccs.size() == 2); // cycle + trivial node
        REQUIRE(toString(sccs) == "[[c,b,a],[d]]");
    }
    SECTION("multiple cycles") {
        auto idA = g.addNode(0);
        auto idB = g.addNode(1);
        auto idC = g.addNode(2);
        auto idD = g.addNode(3);
        auto idE = g.addNode(4);
        auto idF = g.addNode(5);
        auto idG = g.addNode(6);
        auto idH = g.addNode(7);
        auto idI = g.addNode(8);

        g.addEdge(idA, idG);
        g.addEdge(idB, idE);
        g.addEdge(idB, idH);
        g.addEdge(idC, idI);
        g.addEdge(idC, idH);
        g.addEdge(idD, idF);
        g.addEdge(idE, idA);
        g.addEdge(idF, idB);
        g.addEdge(idF, idC);
        g.addEdge(idG, idD);

        REQUIRE(toString(g.computeSccs()) == "[[h],[i],[c],[e,b,f,d,g,a]]");
    }
    SECTION("graph intact after computeSccs") {
        auto idA = g.addNode(0);
        auto idB = g.addNode(1);
        auto idC = g.addNode(2);
        auto idD = g.addNode(3);
        auto idE = g.addNode(4);
        auto idF = g.addNode(5);
        auto idG = g.addNode(6);
        auto idH = g.addNode(7);
        auto idI = g.addNode(8);

        g.addEdge(idA, idB);
        g.addEdge(idB, idC);
        g.addEdge(idC, idH);
        g.addEdge(idC, idD);
        g.addEdge(idD, idE);
        g.addEdge(idE, idF);
        g.addEdge(idE, idB);
        g.addEdge(idE, idC);
        g.addEdge(idF, idG);
        g.addEdge(idG, idF);
        g.addEdge(idH, idI);
        g.addEdge(idI, idH);

        auto expected = "[[i,h],[g,f],[e,d,c,b],[a]]";
        REQUIRE(toString(g.computeSccs()) == expected);
        REQUIRE(toString(g.computeSccs()) == expected);
        REQUIRE(toString(g.computeSccs()) == expected);
    }
}

} // namespace Potassco::Test::Reify
