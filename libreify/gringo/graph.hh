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

#include <forward_list>
#include <vector>

namespace Potassco {
/////////////////////////////////////////////////////////////////////////////////////////
// Graph template
/////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class Graph {
public:
    struct Node;
    using NodeVec = std::vector<Node*>;
    using SCCVec  = std::vector<NodeVec>;

    struct Node {
        friend class Graph;
        template <class... U>
        Node(unsigned phase, U&& ...data);

        Node(const Node& other) = delete;
        Node(Node&& other) noexcept = default;
        Node& operator=(const Node& other) = delete;
        Node& operator=(Node&& other) noexcept = default;
        ~Node() noexcept = default;

        void insertEdge(Node& n);
        auto begin() const -> typename NodeVec::const_iterator;
        auto end()   const -> typename NodeVec::const_iterator;

        // NOLINTNEXTLINE
        T data;

    private:
        NodeVec edges_;
        unsigned visited_;
        typename NodeVec::iterator finished_;
    };

    Graph() = default;
    Graph(Graph&& other) noexcept = default;
    Graph(const Graph& other) = delete;
    Graph& operator=(Graph&& other) noexcept = default;
    Graph& operator=(const Graph&) = delete;
    ~Graph() = default;

    auto tarjan() -> SCCVec;

    template <class... U>
    auto insertNode(U&& ...x) -> Node&;

private:
    unsigned nphase() { return phase_ == 0 ? 1 : 0; }
    using NodeList = std::forward_list<Node>;

    NodeList nodes_;
    unsigned phase_ = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Graph<T>::Node
/////////////////////////////////////////////////////////////////////////////////////////
template <class T>
template <class... U>
Graph<T>::Node::Node(unsigned phase, U&& ...data)
    : data(std::forward<U>(data)...), visited_(phase) {}

template <class T>
void Graph<T>::Node::insertEdge(Node& n) { edges_.emplace_back(&n); }

template <class T>
auto Graph<T>::Node::begin() const -> typename NodeVec::const_iterator { return edges_.begin(); }

template <class T>
auto Graph<T>::Node::end()   const -> typename NodeVec::const_iterator { return edges_.end(); }

/////////////////////////////////////////////////////////////////////////////////////////
// Graph<T> definitions
/////////////////////////////////////////////////////////////////////////////////////////
template <class T>
template <class... U>
auto Graph<T>::insertNode(U&& ...x) -> Node& {
    nodes_.emplace_front(nphase(), std::forward<U>(x)...);
    return nodes_.front();
}

template <class T>
auto Graph<T>::tarjan() -> SCCVec {
    SCCVec sccs;
    NodeVec stack;
    NodeVec trail;

    for (auto& x : nodes_) {
        if (x.visited_ == nphase()) {
            unsigned index = 1;
            auto push = [&stack, &trail, &index](Node& x) {
                x.visited_ = ++index;
                x.finished_ = x.edges_.begin();
                stack.emplace_back(&x);
                trail.emplace_back(&x);
            };
            push(x);

            while (!stack.empty()) {
                auto& y = stack.back();
                auto end = y->edges_.end();

                for (; y->finished_ != end && (*y->finished_)->visited_ != nphase(); ++y->finished_) {
                }

                if (y->finished_ != end) {
                    push(**y->finished_++);
                } else {
                    stack.pop_back();
                    bool root = true;

                    for (auto& z : y->edges_) {
                        if (z->visited_ != phase_ && z->visited_ < y->visited_) {
                            root = false;
                            y->visited_ = z->visited_;
                        }
                    }

                    if (root) {
                        sccs.emplace_back();
                        do {
                            sccs.back().emplace_back(trail.back());
                            trail.back()->visited_ = phase_;
                            trail.pop_back();
                        } while (sccs.back().back() != y);
                    }
                }
            }
        }
    }

    phase_ = nphase();
    return sccs;
}
} // namespace Potassco
