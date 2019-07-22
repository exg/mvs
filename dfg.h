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

double config_weight(const DFG &dfg, const intset &config);
std::tuple<int, int> config_io(const DFG &dfg, const intset &config);
int io_iter_next(const DFG &dfg, const intset &config, int &id, bool &input);

class io_delta_iter {
public:
    io_delta_iter(const DFG &dfg, const intset &config, int u, bool add)
        : dfg_(&dfg)
        , config_(&config)
        , u_(u)
        , add_(add)
        , state_(0)
    {
    }
    int next(bool &input, bool &add);

private:
    const DFG *dfg_;
    const intset *config_;
    int u_;
    bool add_;
    int state_;
};
