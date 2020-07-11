#pragma once

#include "dfg.h"
#include "vset.h"

class IOAnalysis {
public:
    IOAnalysis(const IOSubgraph &config, const intset &nodes_left);
    int num_perm_in() const { return num_perm_in_; }
    int num_perm_out() const { return num_perm_out_; }
    int num_shared_non_perm_out() const { return num_shared_non_perm_out_; }
    vmap<int, double> &get_inputs() { return inputs_; }
    vmap<int, double> &get_rnodes() { return rnodes_; }

    static int num_perm_in(const IOSubgraph &config, const intset &nodes_left);
    static int num_perm_out(const IOSubgraph &config, const intset &nodes_left);

private:
    int num_perm_in_ = 0;
    int num_perm_out_ = 0;
    int num_shared_non_perm_out_ = 0;
    vmap<int, double> inputs_;
    vmap<int, double> rnodes_;
};
