#pragma once

#include "cluster.h"
#include "common.h"
#include "dfg.h"

class VCluster {
public:
    std::vector<int> nodes;

    VCluster(const intset &P)
        : P_(P)
    {
        static_assert(std::is_nothrow_move_constructible<VCluster>::value, "");
    }

    const intset &P() const { return P_; }

private:
    intset P_;
};

class mvs : public IOSubgraph {
public:
    mvs(const IOSubgraph &config)
        : IOSubgraph(config)
    {
    }

    bool disconnected = false;
    int io_weight = 0;
};

class MVSFinder {
public:
    enum class IterType {
        LINEAR,
        LINEAR_REV,
        BINARY_SEARCH,
    };

    MVSFinder(DFG *dfg);
    std::vector<IOSubgraph> enumerate(int max_num_in,
                                      int max_num_out,
                                      IterType itype,
                                      uint8_t flags);
    const intset &nodes() const { return config_.nodes(); }

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
    void link_cluster(const SCluster &cluster);
    void unlink_cluster(const SCluster &cluster);

    DFG *dfg_;
    std::vector<VCluster> v_clusters_;
    std::vector<SCluster> s_clusters_;
    std::vector<SCluster> s_nodes_;
    std::vector<mvs> mvs_vec_;
    std::vector<IOSubgraph> *io_output_;
    IOSubgraph config_;
    IterType itype_;
    uint8_t flags_;
    intset nodes_left_;
    intset clustered_;
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
