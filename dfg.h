#pragma once

#include "intset.h"
#include "vset.h"
#include <iostream>
#include <list>
#include <memory>
#include <string>

class DFG {
public:
    DFG(std::string name, int num_nodes, int frequency);
    static std::unique_ptr<DFG> make_dfg(std::istream &in, bool set_weights);
    void index();

    void add_edge(int u, int v)
    {
        out_list_[u].add(v);
        in_list_[v].add(u);
    }

    void remove_edge(int u, int v)
    {
        out_list_[u].remove(v);
        in_list_[v].remove(u);
    }

    void set_forbidden(int u) { F_.add(u); }

    const std::string &name() const { return name_; }
    int num_nodes() const { return num_nodes_; }
    double weight(int u) const { return weights_[u]; }
    double &weight(int u) { return weights_[u]; }
    const vset<int> &in_edges(int u) const { return in_list_[u]; }
    const vset<int> &out_edges(int u) const { return out_list_[u]; }
    const intset &pred(int u) const { return pred_[u]; }
    const intset &succ(int u) const { return succ_[u]; }
    const intset &forbidden() const { return F_; }

private:
    void dfs_visit(int u, bool *visited, std::list<int> &topo_order);

    std::string name_;
    int num_nodes_;
    int frequency_;

    std::vector<vset<int>> in_list_;
    std::vector<vset<int>> out_list_;

    std::vector<double> weights_;

    std::vector<intset> pred_;
    std::vector<intset> succ_;
    intset F_;
};

class config {
public:
    config(const DFG &dfg)
        : dfg_(&dfg)
        , nodes_(dfg_->num_nodes())
    {
    }
    config(const DFG &dfg, const intset &&nodes)
        : dfg_(&dfg)
        , nodes_(std::move(nodes))
    {
    }

    void add(int u) { nodes_.add(u); }
    void remove(int u) { nodes_.remove(u); }

    const intset &nodes() const { return nodes_; }
    intset pred() const;
    intset succ() const;
    intset closure() const;

protected:
    const DFG *dfg_;
    intset nodes_;
};

class io_config : public config {
public:
    io_config(const DFG &dfg)
        : config(dfg)
    {
        static_assert(std::is_nothrow_move_constructible<io_config>::value, "");
    }

    io_config(const DFG &dfg, const intset &&nodes)
        : config(dfg, std::move(nodes))
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
