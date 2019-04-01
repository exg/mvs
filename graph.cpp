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

Graph::Graph(std::initializer_list<std::pair<int, int>> list)
{
    int num_nodes = 0;
    for (auto &edge : list)
        num_nodes = std::max({
            num_nodes,
            edge.first + 1,
            edge.second + 1,
        });
    nodes_.resize(num_nodes);
    for (auto &edge : list)
        add_edge(edge.first, edge.second);
}

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

void Graph::invert()
{
    for (int i = 0; i < nodes_.size(); i++) {
        unsigned pos = 0;
        auto size = nodes_[i].adj_list.size();
        std::sort(nodes_[i].adj_list.begin(), nodes_[i].adj_list.end());
        for (int u = 0; u < nodes_.size(); u++) {
            if (pos < size && nodes_[i].adj_list[pos] == u)
                pos++;
            else if (u != i)
                nodes_[i].adj_list.push_back(u);
        }
        nodes_[i].adj_list.erase(nodes_[i].adj_list.begin(),
                                 nodes_[i].adj_list.begin() + size);
    }
}

MISFinder::MISFinder(const Graph *graph,
                     bool use_bk,
                     std::function<void(const intset &)> output_cb,
                     std::function<void(const intset &, int, bool)> update_cb)
    : graph_(graph)
    , config_(graph->num_nodes())
    , nodes_left_(graph->num_nodes())
    , f_nodes_(graph->num_nodes())
    , output_cb_(std::move(output_cb))
    , update_cb_(std::move(update_cb))
{
    auto size = graph_->num_nodes();
    for (int i = 0; i < size; i++)
        nodes_left_.add(i);
    if (use_bk)
        bk_visit();
    else {
        num_edges_.resize(size);
        g_num_edges_ = 0;
        for (int i = 0; i < size; i++) {
            config_.add(i);
            num_edges_[i] = graph_->edges(i).size();
            g_num_edges_ += num_edges_[i];
        }
        visit();
    }
}

void MISFinder::visit()
{
    calls_++;

    if (g_num_edges_ == 0) {
        output_cb_(config_);
        count_++;
        return;
    }

    bool is_f_node = false;
    int id = f_nodes_.minimum();
    if (id != -1) {
        f_nodes_.remove(id);
        is_f_node = true;
    } else {
        int max_edges = 0;
        for (const auto &u : nodes_left_) {
            if (num_edges_[u] > max_edges) {
                max_edges = num_edges_[u];
                id = u;
            }
        }
    }
    if (id == -1)
        return;

    nodes_left_.remove(id);

    config_.remove(id);
    update_cb_(config_, id, false);

    bool prune = false;
    g_num_edges_ -= 2 * num_edges_[id];
    for (int v : graph_->edges(id)) {
        num_edges_[v]--;

        if (num_edges_[v] == 0 && !config_.contains(v))
            prune = true;
    }
    if (!prune)
        visit();
    else
        f_nodes_.clear();

    config_.add(id);
    update_cb_(config_, id, true);

    g_num_edges_ += 2 * num_edges_[id];
    if (!is_f_node) {
        for (int v : graph_->edges(id)) {
            num_edges_[v]++;

            if (config_.contains(v)) {
                assert(nodes_left_.contains(v));
                f_nodes_.add(v);
            }
        }
        visit();
    } else {
        for (int v : graph_->edges(id))
            num_edges_[v]++;
    }

    nodes_left_.add(id);
}

static void find_pivot(const Graph &graph,
                       const intset &S,
                       const intset &P,
                       int &best_id,
                       int &best_score)
{
    for (const auto &id : S) {
        int score = 0;
        for (int v : graph.edges(id)) {
            if (P.contains(v))
                score++;
        }
        if (best_id == -1 || score < best_score) {
            best_id = id;
            best_score = score;
        }
    }
}

void MISFinder::bk_visit()
{
    calls_++;

    if (nodes_left_.minimum() == -1 && f_nodes_.minimum() == -1) {
        output_cb_(config_);
        count_++;
        return;
    }

    intset P(nodes_left_);
    intset X(f_nodes_);

    int best_score;
    int best_id = -1;
    find_pivot(*graph_, nodes_left_, nodes_left_, best_id, best_score);
    find_pivot(*graph_, f_nodes_, nodes_left_, best_id, best_score);

    auto &edges = graph_->edges(best_id);
    for (int j = 0; j <= edges.size(); j++) {
        int id = j < edges.size() ? edges[j] : best_id;

        if (!P.contains(id))
            continue;

        nodes_left_ = P;
        f_nodes_ = X;

        nodes_left_.remove(id);
        for (int v : graph_->edges(id)) {
            nodes_left_.remove(v);
            f_nodes_.remove(v);
        }

        config_.add(id);
        update_cb_(config_, id, true);

        bk_visit();

        config_.remove(id);
        update_cb_(config_, id, false);

        P.remove(id);
        X.add(id);
    }
}
