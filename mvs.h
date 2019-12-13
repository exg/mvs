#pragma once

#include "cluster.h"
#include "common.h"
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

class mvs : public io_config {
public:
    mvs(const io_config &config)
        : io_config(config)
    {
    }

    bool disconnected = false;
    int io_weight = 0;
};

enum class iter_type {
    LINEAR,
    LINEAR_REV,
    BINARY_SEARCH,
};

class mvs_finder {
public:
    mvs_finder(DFG *dfg);
    std::vector<io_config> mvs_enum(int max_num_in,
                                    int max_num_out,
                                    iter_type itype,
                                    uint8_t flags);
    const DFG &dfg() const { return *dfg_; }
    const intset &nodes() const { return config_.nodes(); }
    const intset &nodes_left() const { return nodes_left_; }
    const vset<int> &inputs() const { return config_.inputs(); }
    const vset<int> &outputs() const { return config_.outputs(); }

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
    void update_config(int id, bool add);

    DFG *dfg_;
    std::vector<v_cluster> v_clusters_;
    std::vector<s_cluster> s_clusters_;
    std::vector<s_cluster> s_nodes_;
    std::vector<mvs> mvs_vec_;
    std::vector<io_config> *io_output_;
    io_config config_;
    iter_type itype_;
    uint8_t flags_;
    intset nodes_left_;
    intset collapsed_;
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
        nlohmann::json json = {
            {"count", count_},
            {"min_weight", min_weight},
            {"calls", calls_},
            {"pruned", pruned_},
        };
        std::cerr << json.dump() << std::endl;
    }
};

class io_analysis {
public:
    io_analysis(const mvs_finder &finder);
    int num_perm_in() const { return num_perm_in_; }
    int num_perm_out() const { return num_perm_out_; }
    int num_shared_non_perm_out() const { return num_shared_non_perm_out_; }
    double best_input_weights(int n);
    double best_rnode_weights(int n);

private:
    const mvs_finder *finder_;
    int num_perm_in_ = 0;
    int num_perm_out_ = 0;
    int num_shared_non_perm_out_ = 0;
    vmap<int, double> inputs_;
    vmap<int, double> rnodes_;
};
