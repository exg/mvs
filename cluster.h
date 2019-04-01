#pragma once

#include "dfg.h"

class SCluster {
public:
    SCluster(std::vector<std::pair<int, double>> &&nodes,
             std::vector<std::pair<int, int>> &&edges,
             int src,
             int dst)
        : nodes_(std::move(nodes))
        , edges_(std::move(edges))
        , src_(src)
        , dst_(dst)
    {
        static_assert(std::is_nothrow_move_constructible<SCluster>::value, "");
    }

    void expand(IOSubgraph &config) const
    {
        if (config.nodes().contains(dst_))
            for (auto &node : nodes_)
                config.add(node.first);
    }

    void contract(IOSubgraph &config) const
    {
        if (config.nodes().contains(dst_))
            for (auto &node : nodes_)
                config.remove(node.first);
    }

    int src() const { return src_; }
    int dst() const { return dst_; }
    const std::vector<std::pair<int, double>> &nodes() const { return nodes_; }
    const std::vector<std::pair<int, int>> &edges() const { return edges_; }

private:
    std::vector<std::pair<int, double>> nodes_;
    std::vector<std::pair<int, int>> edges_;
    int src_;
    int dst_;
};

std::vector<SCluster> scluster_enumerate(const DFG &dfg);
std::vector<SCluster> snode_enumerate(const Subgraph &subgraph,
                                      const std::vector<SCluster> &s_clusters);
