#pragma once

#include "intset.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

class Node {
public:
    void add_edge(int v)
    {
        adj_list_.push_back(v);
        num_edges_++;
    }

    void invert(int id, unsigned n)
    {
        unsigned pos = 0;
        unsigned size = adj_list_.size();
        std::sort(adj_list_.begin(), adj_list_.end());
        for (int u = 0; u < n; u++) {
            if (pos < size && adj_list_[pos] == u)
                pos++;
            else if (u != id)
                adj_list_.push_back(u);
        }

        adj_list_.erase(adj_list_.begin(), adj_list_.begin() + size);
        num_edges_ = adj_list_.size();
    }

    const std::vector<int> &edges() const { return adj_list_; }
    int num_edges() const { return num_edges_; }
    int &num_edges() { return num_edges_; }

private:
    std::vector<int> adj_list_;
    int num_edges_ = 0;
};

class Graph {
public:
    Graph(int num_nodes) { nodes_.resize(num_nodes); }
    static std::unique_ptr<Graph> make_graph(std::istream &in);

    void add_edge(int u, int v)
    {
        auto u_edges = nodes_[u].edges();
        if (std::find(u_edges.begin(), u_edges.end(), v) == u_edges.end()) {
            nodes_[u].add_edge(v);
            nodes_[v].add_edge(u);
            num_edges_ += 2;
        }
    }

    void invert()
    {
        num_edges_ = 0;
        for (int i = 0; i < nodes_.size(); i++) {
            nodes_[i].invert(i, nodes_.size());
            num_edges_ += nodes_[i].edges().size();
        }
    }

    const Node &node(int u) const { return nodes_[u]; }
    int num_nodes() const { return nodes_.size(); }
    int num_edges() const { return num_edges_; }
    int &num_edges() { return num_edges_; }
    int &num_edges(int u) { return nodes_[u].num_edges(); }

private:
    std::vector<Node> nodes_;
    int num_edges_ = 0;
};

class mis_finder {
public:
    mis_finder(Graph *graph)
        : graph_(graph)
        , config_(graph->num_nodes())
        , nodes_left_(graph->num_nodes())
        , f_nodes_(graph->num_nodes())
    {
    }

    std::pair<unsigned, unsigned> visit(
        bool use_bk,
        const std::function<void(const intset &)> &output_cb,
        const std::function<void(const intset &)> &verify_cb,
        const std::function<void(const intset &, int, bool)> &update_cb);

private:
    void visit_();
    void bk_visit_();

    std::function<void(const intset &)> output_cb_;
    std::function<void(const intset &)> verify_cb_;
    std::function<void(const intset &, int, bool)> update_cb_;

    Graph *graph_;
    intset config_;
    intset nodes_left_;
    intset f_nodes_;
    unsigned count_;
    unsigned calls_;
};
