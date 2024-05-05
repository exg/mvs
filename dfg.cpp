/* Copyright (C) 2013-2019 Emanuele Giaquinta

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include "dfg.h"
#include "common.h"
#include "vset.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <istream>
#include <list>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>

DFG::DFG(std::initializer_list<std::pair<int, int>> list)
{
    int num_nodes = 0;
    for (auto &edge : list)
        num_nodes = std::max({
            num_nodes,
            edge.first + 1,
            edge.second + 1,
        });
    for (int i = 0; i < num_nodes; i++)
        nodes_.emplace_back(num_nodes);
    for (auto &edge : list)
        add_edge(edge.first, edge.second);
}

std::unique_ptr<DFG> DFG::make_dfg(std::istream &in, bool set_weights)
{
    std::string line;
    int nodes = 0;
    int freq = 0;
    std::unique_ptr<DFG> dfg = nullptr;

    while (std::getline(in, line, '\n')) {
        auto fields = split(line, ' ');
        auto size = fields.size();
        if (fields[0] != "p" && !dfg)
            throw std::runtime_error("invalid line");
        if (fields[0] == "p") {
            if (size < 6 || !parse_integer(fields[2], nodes, 0, INT_MAX) ||
                !parse_integer(fields[5], freq, 0, INT_MAX))
                throw std::runtime_error("invalid line");

            dfg = std::make_unique<DFG>(fields[4], nodes, freq);
        } else if (fields[0] == "e") {
            int u;
            int v;

            if (size < 3 || !parse_integer(fields[1], u, 1, nodes) ||
                !parse_integer(fields[2], v, 1, nodes))
                throw std::runtime_error("invalid line");

            dfg->add_edge(u - 1, v - 1);
        } else if (fields[0] == "n") {
            int id;
            int is_forbidden;

            if (size < 4 || !parse_integer(fields[1], id, 1, nodes) ||
                !parse_integer(fields[3], is_forbidden, 0, 1))
                throw std::runtime_error("invalid line");

            if (is_forbidden)
                dfg->set_forbidden(id - 1);

            if (set_weights)
                dfg->weight(id - 1) = strtod(fields[2].c_str(), nullptr);
        }
    }
    double max_weight = 0;
    for (int i = 0; i < dfg->num_nodes(); i++)
        max_weight += dfg->weight(i);
    assert(max_weight <= INT_MAX);
    dfg->index();
    return dfg;
}

void DFG::index()
{
    // compute a topological ordering, just in case
    std::list<int> topo_order;
    DFSVisitor visitor(*this, [this, &topo_order](int u) {
        nodes_[u].pred.clear();
        nodes_[u].succ.clear();
        topo_order.push_front(u);
    });

    // compute pred and succ sets for each node
    for (auto &u : topo_order) {
        // if (F_.contains(u))
        //   continue;

        for (auto &v : out_edges(u)) {
            nodes_[v].pred.add(nodes_[u].pred);
            nodes_[v].pred.add(u);
        }
    }

    for (auto it = topo_order.rbegin(); it != topo_order.rend(); it++) {
        auto u = *it;
        // if (F_.contains(u))
        //   continue;

        for (auto &v : out_edges(u)) {
            nodes_[u].succ.add(nodes_[v].succ);
            nodes_[u].succ.add(v);
        }
    }
}

intset DFG::forbidden() const
{
    intset s(num_nodes());
    for (int i = 0; i < num_nodes(); i++) {
        if (is_forbidden(i) || in_edges(i).empty() || out_edges(i).empty())
            s.add(i);
    }
    return s;
}

void DFSVisitor::i_visit(const DFG &dfg,
                         int u,
                         const std::function<void(int)> &visit_cb)
{
    struct Frame {
        int node;
        vset<int>::const_iterator it;
    };
    std::stack<Frame> stack;
    stack.push({u, dfg.out_edges(u).begin()});
    visited_[u] = true;
    while (!stack.empty()) {
        auto frame = stack.top();
        stack.pop();
        int u = frame.node;
        auto it = frame.it;
        while (it != dfg.out_edges(u).end()) {
            int v = *it++;
            if (!visited_[v]) {
                visited_[v] = true;
                stack.push({u, it});
                u = v;
                it = dfg.out_edges(u).begin();
            }
        }
        visit_cb(u);
    }
}

intset Subgraph::pred() const
{
    intset out(dfg_->num_nodes());
    for (const auto &u : nodes_) {
        out.add(dfg_->pred(u));
    }
    return out.remove(nodes_);
}

intset Subgraph::succ() const
{
    intset out(dfg_->num_nodes());
    for (const auto &u : nodes_) {
        out.add(dfg_->succ(u));
    }
    return out.remove(nodes_);
}

intset Subgraph::closure() const
{
    return nodes_ | pred() & succ();
}

void IOSubgraph::init_weight()
{
    weight_ = 0;
    for (const auto &u : nodes_) {
        weight_ += dfg_->weight(u);
    }
}

// true iff at least one successor of 'u' different from 'z' belongs
// to the subgraph 'config'
static bool has_internal_successor(const DFG &dfg,
                                   const intset &config,
                                   int u,
                                   int z)
{
    for (auto &v : dfg.out_edges(u)) {
        if (v != z && config.contains(v))
            return true;
    }
    return false;
}

// true iff at least one successor of 'u' different from 'z' does
// not belong to the subgraph 'config'
static bool has_external_successor(const DFG &dfg,
                                   const intset &config,
                                   int u,
                                   int z)
{
    for (auto &v : dfg.out_edges(u)) {
        if (v != z && !config.contains(v))
            return true;
    }
    return false;
}

static const bool VERIFY = false;

void IOSubgraph::init_io()
{
    inputs_.clear();
    outputs_.clear();

    for (int i = 0; i < dfg_->num_nodes(); i++) {
        if (!nodes_.contains(i)) {
            if (has_internal_successor(*dfg_, nodes_, i, i))
                inputs_.add(i);
        } else {
            if (has_external_successor(*dfg_, nodes_, i, i))
                outputs_.add(i);
        }
    }
}

void IOSubgraph::update_io(int u, bool add)
{
    if (has_internal_successor(*dfg_, nodes_, u, u)) {
        if (!add)
            inputs_.add(u);
        else
            inputs_.remove(u);
    }

    if (has_external_successor(*dfg_, nodes_, u, u)) {
        if (add)
            outputs_.add(u);
        else
            outputs_.remove(u);
    }

    for (auto &v : dfg_->in_edges(u)) {
        if (v >= dfg_->num_nodes() || !nodes_.contains(v)) {
            if (!has_internal_successor(*dfg_, nodes_, v, u)) {
                if (add)
                    inputs_.add(v);
                else
                    inputs_.remove(v);
            }
        } else {
            if (!has_external_successor(*dfg_, nodes_, v, u)) {
                if (!add)
                    outputs_.add(v);
                else
                    outputs_.remove(v);
            }
        }
    }

    if (VERIFY) {
        vset<int> inputs;
        vset<int> outputs;
        for (int i = 0; i < dfg_->num_nodes(); i++) {
            if (!nodes_.contains(i)) {
                if (has_internal_successor(*dfg_, nodes_, i, i))
                    inputs.add(i);
            } else {
                if (has_external_successor(*dfg_, nodes_, i, i))
                    outputs.add(i);
            }
        }
        assert(inputs_.size() == inputs.size() &&
               outputs_.size() == outputs.size());
        for (auto &input : inputs)
            assert(std::find(inputs_.begin(), inputs_.end(), input) !=
                   inputs_.end());
        for (auto &output : outputs)
            assert(std::find(outputs_.begin(), outputs_.end(), output) !=
                   outputs_.end());
    }
}
