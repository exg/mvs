#pragma once

#include "cluster.h"
#include "dfg.h"

class v_cluster {
public:
    std::vector<int> nodes;

    v_cluster(const intset &P)
        : P_(P)
    {
        static_assert(std::is_nothrow_move_constructible<v_cluster>::value, "");
    }

    const intset &P() const { return P_; }

private:
    intset P_;
};

class mvs {
public:
    mvs(const intset &config, int num_in, int num_out, double weight)
        : config_(config)
        , num_in_(num_in)
        , num_out_(num_out)
        , weight_(weight)
    {
        static_assert(std::is_nothrow_move_constructible<mvs>::value, "");
    }

    const intset &config() const { return config_; }
    int num_in() const { return num_in_; }
    int num_out() const { return num_out_; }
    double weight() const { return weight_; }
    bool disconnected = false;
    int io_weight = 0;

private:
    intset config_;
    int num_in_;
    int num_out_;
    double weight_;
};

enum class iter_type { LINEAR, LINEAR_REV, BINARY_SEARCH };

class mvs_finder {
public:
    mvs_finder(DFG *dfg);
    std::vector<mvs> mvs_enum(int max_num_in,
                              int max_num_out,
                              iter_type itype,
                              uint8_t flags);
    const DFG &dfg() const { return *dfg_; }
    const intset &config() const { return config_; }
    const intset &nodes_left() const { return nodes_left_; }
    vmap<int, double> &inputs() { return inputs_; }
    vset<int> &outputs() { return outputs_; }

private:
    int find_best_recursion_node(int max_num_in,
                                 int max_num_out,
                                 int num_perm_in,
                                 int num_perm_out);
    void visit(double dels,
               bool single,
               int &max_weight,
               int max_num_in,
               int max_num_out);
    int find_mvsio_(mvs &mvs,
                    bool single,
                    int max_io_weight,
                    int max_num_in,
                    int max_num_out);
    void find_mvsio(mvs &mvs,
                    bool single,
                    int max_io_weight,
                    int max_num_in,
                    int max_num_out);
    void link_cluster(const s_cluster &cluster);
    void unlink_cluster(const s_cluster &cluster);
    void add_config();
    void verify_config();
    void update_config(int id, bool add);

    DFG *dfg_;
    std::vector<v_cluster> v_clusters_;
    std::vector<s_cluster> s_clusters_;
    std::vector<s_cluster> s_nodes_;
    std::vector<mvs> mvs_vec_;
    std::vector<mvs> *io_output_;
    iter_type itype_;
    uint8_t flags_;
    intset config_;
    intset nodes_left_;
    intset collapsed_;
    int num_in_;
    int num_out_;
    vmap<int, double> inputs_;
    vset<int> outputs_;
    unsigned count_;
    unsigned calls_;
    unsigned pruned_[3];

    void reset_stats()
    {
        count_ = 0;
        calls_ = 0;
        for (int i = 0; i < 3; i++)
            pruned_[i] = 0;
    }

    void dump_stats(int min_weight)
    {
        fprintf(stderr,
                "  MIN-WEIGHT=%d COUNT=%d NUM-CALLS=%d PRUNED[1]=%d "
                "PRUNED[2]=%d PRUNED[3]=%d\n",
                min_weight,
                count_,
                calls_,
                pruned_[0],
                pruned_[1],
                pruned_[2]);
    }
};

class io_analysis {
public:
    io_analysis(mvs_finder &finder);
    int num_perm_in() const { return num_perm_in_; }
    int num_perm_out() const { return num_perm_out_; }
    int num_shared_non_perm_out() const { return num_shared_non_perm_out_; }
    double best_input_weights(int n);
    double best_rnode_weights(int n);

private:
    mvs_finder *finder_;
    int num_perm_in_ = 0;
    int num_perm_out_ = 0;
    int num_shared_non_perm_out_ = 0;
    vmap<int, double> rnodes_;
};
