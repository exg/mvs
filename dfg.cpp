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
#include <cassert>
#include <climits>
#include <memory>
#include <stdexcept>
#include <utility>

DFG::DFG(std::string name, int num_nodes, int frequency)
    : name_(std::move(name))
    , num_nodes_(num_nodes)
    , frequency_(frequency)
    , F_(num_nodes)
{
    in_list_.resize(num_nodes);
    out_list_.resize(num_nodes);
    weights_.resize(num_nodes, 1);

    for (int i = 0; i < num_nodes; i++) {
        pred_.emplace_back(num_nodes);
        succ_.emplace_back(num_nodes);
    }
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

void DFG::dfs_visit(int u, bool *visited, std::list<int> &topo_order)
{
    for (auto &v : out_list_[u]) {
        if (!visited[v]) {
            visited[v] = true;
            dfs_visit(v, visited, topo_order);
        }
    }

    topo_order.push_front(u);
}

void DFG::index()
{
    for (int i = 0; i < num_nodes_; i++) {
        if (in_list_[i].empty())
            F_.add(i);

        if (out_list_[i].empty())
            F_.add(i);
    }

    // compute a topological ordering, just in case
    auto visited = std::make_unique<bool[]>(num_nodes_);
    std::list<int> topo_order;
    for (int i = 0; i < num_nodes_; i++)
        if (!visited[i])
            dfs_visit(i, visited.get(), topo_order);

    // compute pred and succ sets for each node
    for (auto &u : topo_order) {
        // if (F_.contains(u))
        //   continue;

        for (auto &v : out_list_[u]) {
            pred_[v].add(pred_[u]);
            pred_[v].add(u);
        }
    }

    for (auto it = topo_order.rbegin(); it != topo_order.rend(); it++) {
        auto u = *it;
        // if (F_.contains(u))
        //   continue;

        for (auto &v : out_list_[u]) {
            succ_[u].add(succ_[v]);
            succ_[u].add(v);
        }
    }
}

intset config::pred() const
{
    intset out(dfg_->num_nodes());
    int u = 0;
    for (;;) {
        u = nodes_.find_next(u);
        if (u == -1)
            break;

        out.add_difference(dfg_->pred(u), nodes_);
        u++;
    }

    return out;
}

intset config::succ() const
{
    intset out(dfg_->num_nodes());
    int u = 0;
    for (;;) {
        u = nodes_.find_next(u);
        if (u == -1)
            break;

        out.add_difference(dfg_->succ(u), nodes_);
        u++;
    }

    return out;
}

intset config::closure() const
{
    auto out = pred();
    out.intersect(succ());
    out.add(nodes_);
    return out;
}

void io_config::init_weight()
{
    weight_ = 0;
    int u = 0;
    for (;;) {
        u = nodes_.find_next(u);
        if (u == -1)
            break;

        weight_ += dfg_->weight(u);
        u++;
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

void io_config::init_io()
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

void io_config::update_io(int u, bool add)
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
