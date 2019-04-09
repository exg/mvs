#pragma once

#include "common.h"
#include "dfg.h"

class s_cluster {
public:
    s_cluster(std::vector<int> &&nodes,
              std::vector<std::pair<int, int>> &&edges,
              int src,
              int dst)
        : nodes_(std::move(nodes))
        , edges_(std::move(edges))
        , src_(src)
        , dst_(dst)
    {
        static_assert(std::is_nothrow_move_constructible<s_cluster>::value, "");
    }

    void expand(intset &config) const
    {
        if (config.contains(dst_))
            for (auto &node : nodes_)
                config.add(node);
    }

    void contract(intset &config) const
    {
        if (config.contains(dst_))
            for (auto &node : nodes_)
                config.remove(node);
    }

    int src() const { return src_; }
    int dst() const { return dst_; }
    const std::vector<int> &nodes() const { return nodes_; }
    const std::vector<std::pair<int, int>> &edges() const { return edges_; }

private:
    std::vector<int> nodes_;
    std::vector<std::pair<int, int>> edges_;
    int src_;
    int dst_;
};

std::vector<s_cluster> scluster_enum(const DFG &dfg);
std::vector<s_cluster> snode_enum(const DFG &dfg,
                                  const intset &config,
                                  const std::vector<s_cluster> &s_clusters);
