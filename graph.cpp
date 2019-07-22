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

#include "graph.h"
#include "common.h"
#include <cassert>
#include <climits>
#include <string>

std::unique_ptr<Graph> Graph::make_graph(std::istream &in)
{
    std::string line;
    int nodes = 0;
    std::unique_ptr<Graph> graph = nullptr;

    while (std::getline(in, line, '\n')) {
        auto fields = split(line, ' ');
        auto size = fields.size();
        if (fields[0] != "p" && !graph)
            throw std::runtime_error("invalid line");
        if (fields[0] == "p") {
            if (size < 3 || !parse_integer(fields[2], nodes, 0, INT_MAX))
                throw std::runtime_error("invalid line");

            graph = std::make_unique<Graph>(nodes);
        } else if (fields[0] == "e") {
            int u;
            int v;

            if (size < 3 || !parse_integer(fields[1], u, 1, nodes) ||
                !parse_integer(fields[2], v, 1, nodes))
                throw std::runtime_error("invalid line");

            graph->add_edge(u - 1, v - 1);
        }
    }
    return graph;
}

std::pair<unsigned, unsigned> mis_finder::visit(
    bool use_bk,
    const std::function<void(const intset &)> &output_cb,
    const std::function<void(const intset &)> &verify_cb,
    const std::function<void(const intset &, int, bool)> &update_cb)
{
    auto size = graph_->num_nodes();
    config_.clear();
    nodes_left_.clear();
    f_nodes_.clear();
    count_ = 0;
    calls_ = 0;
    output_cb_ = output_cb;
    verify_cb_ = verify_cb;
    update_cb_ = update_cb;
    for (int i = 0; i < size; i++)
        nodes_left_.add(i);
    if (use_bk)
        bk_visit_();
    else {
        for (int i = 0; i < size; i++)
            config_.add(i);
        visit_();
    }
    return { count_, calls_ };
}

void mis_finder::visit_()
{
    calls_++;

    if (graph_->num_edges() == 0) {
        output_cb_(config_);
        count_++;
        return;
    }

    verify_cb_(config_);

    bool prune;
    bool is_f_node = false;
    int id = f_nodes_.minimum();
    if (id != -1) {
        f_nodes_.remove(id);
        is_f_node = true;
    } else {
        int max_edges = 0;
        int u = 0;
        for (;;) {
            u = nodes_left_.find_next(u);
            if (u == -1)
                break;

            const Node &node = graph_->node(u);
            if (node.num_edges() > max_edges) {
                max_edges = node.num_edges();
                id = u;
            }
            u++;
        }
    }
    if (id == -1)
        return;

    nodes_left_.remove(id);
    const Node &node = graph_->node(id);

    config_.remove(id);
    update_cb_(config_, id, false);

    prune = false;
    graph_->num_edges() -= 2 * node.num_edges();
    for (int v : node.edges()) {
        graph_->num_edges(v)--;

        if (graph_->node(v).num_edges() == 0 && !config_.contains(v))
            prune = true;
    }
    if (!prune)
        visit_();
    else
        f_nodes_.clear();

    config_.add(id);
    update_cb_(config_, id, true);

    graph_->num_edges() += 2 * node.num_edges();
    for (int v : node.edges()) {
        graph_->num_edges(v)++;

        if (!is_f_node)
            if (config_.contains(v)) {
                assert(nodes_left_.contains(v));
                f_nodes_.add(v);
            }
    }
    if (!is_f_node)
        visit_();

    nodes_left_.add(id);
}

static void find_pivot(const Graph &graph,
                       const intset &S,
                       const intset &P,
                       int &best_id,
                       int &best_score)
{
    int id = 0;
    for (;;) {
        id = S.find_next(id);
        if (id == -1)
            break;

        const Node &node = graph.node(id);
        int score = 0;
        for (int v : node.edges()) {
            if (P.contains(v))
                score++;
        }
        if (best_id == -1 || score < best_score) {
            best_id = id;
            best_score = score;
        }
        id++;
    }
}

void mis_finder::bk_visit_()
{
    calls_++;

    if (nodes_left_.minimum() == -1 && f_nodes_.minimum() == -1) {
        output_cb_(config_);
        count_++;
        return;
    }

    verify_cb_(config_);

    intset P(nodes_left_);
    intset X(f_nodes_);

    int best_score;
    int best_id = -1;
    find_pivot(*graph_, nodes_left_, nodes_left_, best_id, best_score);
    find_pivot(*graph_, f_nodes_, nodes_left_, best_id, best_score);

    const Node &b_node = graph_->node(best_id);
    for (int j = 0; j <= b_node.edges().size(); j++) {
        int id = j < b_node.edges().size() ? b_node.edges()[j] : best_id;

        if (!P.contains(id))
            continue;

        const Node &node = graph_->node(id);

        nodes_left_ = P;
        f_nodes_ = X;

        nodes_left_.remove(id);
        for (int v : node.edges()) {
            nodes_left_.remove(v);
            f_nodes_.remove(v);
        }

        config_.add(id);
        update_cb_(config_, id, true);

        bk_visit_();

        config_.remove(id);
        update_cb_(config_, id, false);

        P.remove(id);
        X.add(id);
    }
}
