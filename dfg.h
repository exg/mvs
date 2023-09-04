#pragma once

#include "intset.h"
#include "vset.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>

class DFG {
    struct Node {
        Node(int num_nodes)
            : pred(num_nodes)
            , succ(num_nodes)
        {
        }

        vset<int> in_list;
        vset<int> out_list;
        double weight = 1;
        bool forbidden = false;
        intset pred;
        intset succ;
    };

public:
    DFG(std::string name, int num_nodes, int frequency)
        : name_(std::move(name))
        , frequency_(frequency)
    {
        for (int i = 0; i < num_nodes; i++)
            nodes_.emplace_back(num_nodes);
    }
    DFG(std::initializer_list<std::pair<int, int>> list);
    static std::unique_ptr<DFG> make_dfg(std::istream &in, bool set_weights);

    void add_edge(int u, int v)
    {
        nodes_[u].out_list.add(v);
        nodes_[v].in_list.add(u);
    }
    void remove_edge(int u, int v)
    {
        nodes_[u].out_list.remove(v);
        nodes_[v].in_list.remove(u);
    }
    void set_forbidden(int u) { nodes_[u].forbidden = true; }
    void index();

    const std::string &name() const { return name_; }
    int num_nodes() const { return nodes_.size(); }
    double weight(int u) const { return nodes_[u].weight; }
    double &weight(int u) { return nodes_[u].weight; }
    const vset<int> &in_edges(int u) const { return nodes_[u].in_list; }
    const vset<int> &out_edges(int u) const { return nodes_[u].out_list; }
    const intset &pred(int u) const { return nodes_[u].pred; }
    const intset &succ(int u) const { return nodes_[u].succ; }
    bool is_forbidden(int u) const { return nodes_[u].forbidden; }
    intset forbidden() const;

private:
    std::string name_;
    int frequency_ = 0;

    std::vector<Node> nodes_;
};

class DFSVisitor {
public:
    DFSVisitor(const DFG &dfg, const std::function<void(int)> &visit_cb)
        : visited_(std::make_unique<bool[]>(dfg.num_nodes()))
    {
        for (int i = 0; i < dfg.num_nodes(); i++)
            if (!visited_[i])
                i_visit(dfg, i, visit_cb);
    }

private:
    void r_visit(const DFG &dfg,
                 int u,
                 const std::function<void(int)> &visit_cb)
    {
        visited_[u] = true;
        for (auto &v : dfg.out_edges(u))
            if (!visited_[v])
                r_visit(dfg, v, visit_cb);
        visit_cb(u);
    }

    void i_visit(const DFG &dfg,
                 int u,
                 const std::function<void(int)> &visit_cb);

    std::unique_ptr<bool[]> visited_;
};

class Subgraph {
public:
    Subgraph(const DFG &dfg)
        : dfg_(&dfg)
        , nodes_(dfg_->num_nodes())
    {
    }
    Subgraph(const DFG &dfg, const intset &&nodes)
        : dfg_(&dfg)
        , nodes_(std::move(nodes))
    {
    }

    void add(int u) { nodes_.add(u); }
    void remove(int u) { nodes_.remove(u); }

    const DFG &dfg() const { return *dfg_; }
    const intset &nodes() const { return nodes_; }
    intset pred() const;
    intset succ() const;
    intset closure() const;

protected:
    const DFG *dfg_;
    intset nodes_;
};

class IOSubgraph : public Subgraph {
public:
    IOSubgraph(const DFG &dfg)
        : Subgraph(dfg)
    {
        static_assert(std::is_nothrow_move_constructible<IOSubgraph>::value,
                      "");
    }

    IOSubgraph(const DFG &dfg, const intset &&nodes)
        : Subgraph(dfg, std::move(nodes))
    {
        init_io();
        init_weight();
    }

    const vset<int> &inputs() const { return inputs_; }
    const vset<int> &outputs() const { return outputs_; }
    int num_in() const { return inputs_.size(); }
    int num_out() const { return outputs_.size(); }
    double weight() const { return weight_; }

    void set(const intset &nodes)
    {
        nodes_ = nodes;
        init_io();
        init_weight();
    }
    void add(int u)
    {
        nodes_.add(u);
        update_io(u, true);
        weight_ += dfg_->weight(u);
    }
    void remove(int u)
    {
        nodes_.remove(u);
        update_io(u, false);
        weight_ -= dfg_->weight(u);
    }

private:
    void init_weight();
    void init_io();
    void update_io(int u, bool add);

    vset<int> inputs_;
    vset<int> outputs_;
    double weight_ = 0;
};
