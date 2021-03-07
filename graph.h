#pragma once

#include "intset.h"
#include "vset.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

class Graph {
    struct Node {
        vset<int> adj_list;
    };

public:
    Graph(int num_nodes) { nodes_.resize(num_nodes); }
    Graph(std::initializer_list<std::pair<int, int>> list);
    static std::unique_ptr<Graph> make_graph(std::istream &in);

    void add_edge(int u, int v)
    {
        nodes_[u].adj_list.add(v);
        nodes_[v].adj_list.add(u);
    }
    void remove_edge(int u, int v)
    {
        nodes_[u].adj_list.remove(v);
        nodes_[v].adj_list.remove(u);
    }
    void invert();

    int num_nodes() const { return nodes_.size(); }
    int num_edges() const
    {
        int num_edges = 0;
        for (int i = 0; i < num_nodes(); i++)
            num_edges += edges(i).size();
        return num_edges;
    }
    const vset<int> &edges(int u) const { return nodes_[u].adj_list; }

private:
    std::vector<Node> nodes_;
};

class MISFinderBase {
public:
    MISFinderBase(const Graph *graph,
                  std::function<void(const intset &)> output_cb,
                  std::function<void(const intset &, int, bool)> update_cb);
    virtual ~MISFinderBase() = default;

    unsigned get_count() const { return count_; }
    unsigned get_calls() const { return calls_; }

protected:
    const Graph *graph_;
    intset config_;
    intset nodes_left_;
    intset f_nodes_;
    unsigned count_ = 0;
    unsigned calls_ = 0;

    std::function<void(const intset &)> output_cb_;
    std::function<void(const intset &, int, bool)> update_cb_;
};

class MISFinder : public MISFinderBase {
public:
    MISFinder(const Graph *graph,
              std::function<void(const intset &)> output_cb,
              std::function<void(const intset &, int, bool)> update_cb);

private:
    void visit();

    std::vector<int> num_edges_;
    int g_num_edges_;
};

class MISFinderBK : public MISFinderBase {
public:
    MISFinderBK(const Graph *graph,
                std::function<void(const intset &)> output_cb,
                std::function<void(const intset &, int, bool)> update_cb);

private:
    void visit();
};
