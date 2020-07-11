#pragma once

#include "dfg.h"
#include "vset.h"

class IOAnalysis {
public:
    IOAnalysis(const IOSubgraph &config, const intset &nodes_left);
    int num_perm_in() const { return num_perm_in_; }
    int num_perm_out() const { return num_perm_out_; }
    int num_shared_non_perm_out() const { return num_shared_non_perm_out_; }
    double best_input_weights(int n);
    double best_rnode_weights(int n);

    static int num_perm_in(const IOSubgraph &config, const intset &nodes_left);
    static int num_perm_out(const IOSubgraph &config, const intset &nodes_left);

private:
    const IOSubgraph &config_;
    int num_perm_in_ = 0;
    int num_perm_out_ = 0;
    int num_shared_non_perm_out_ = 0;
    vmap<int, double> inputs_;
    vmap<int, double> rnodes_;
};
