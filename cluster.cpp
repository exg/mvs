/* Copyright (C) 2013-2019 Emanuele Giaquinta

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include "cluster.h"
#include "dfg.h"
#include "vs.h"
#include <utility>
#include <vector>

std::vector<SCluster> scluster_enumerate(const DFG &dfg)
{
    std::vector<SCluster> output;
    std::vector<IOSubgraph> subgraphs;
    vs_enumerate(dfg, 1, 1, [&subgraphs](const IOSubgraph &subgraph) {
        if (subgraph.nodes().size() == 1)
            return;
        for (auto it = subgraphs.begin(); it != subgraphs.end();) {
            if (subgraph.nodes().is_subset_of((*it).nodes()))
                return;
            if ((*it).nodes().is_subset_of(subgraph.nodes()))
                it = subgraphs.erase(it);
            else
                it++;
        }
        subgraphs.emplace_back(subgraph);
    });

    for (const auto &subgraph : subgraphs) {
        std::vector<std::pair<int, double>> nodes;
        std::vector<std::pair<int, int>> edges;
        int vi = -1;
        int vo = -1;
        for (const auto &u : subgraph.nodes()) {
            for (auto &v : dfg.out_edges(u)) {
                if (subgraph.nodes().contains(v))
                    edges.emplace_back(u, v);
                else
                    vo = u;
            }

            if (u != vo)
                nodes.emplace_back(u, dfg.weight(u));

            for (auto &v : dfg.in_edges(u)) {
                if (v >= dfg.num_nodes() || !subgraph.nodes().contains(v)) {
                    edges.emplace_back(v, u);
                    vi = v;
                }
            }
        }

        output.emplace_back(std::move(nodes), std::move(edges), vi, vo);
    }
    return output;
}

std::vector<SCluster> snode_enumerate(const Subgraph &subgraph,
                                      const std::vector<SCluster> &s_clusters)
{
    const DFG &dfg = subgraph.dfg();
    intset nodes = subgraph.nodes();
    for (auto &cluster : s_clusters)
        nodes.remove(cluster.dst());
    std::vector<SCluster> output;
    for (const auto &i : nodes)
        if (dfg.in_edges(i).size() == 1 && dfg.out_edges(i).size() == 1 &&
            !subgraph.nodes().contains(dfg.in_edges(i)[0]) &&
            subgraph.nodes().contains(dfg.out_edges(i)[0])) {
            int pred = dfg.in_edges(i)[0];
            int succ = dfg.out_edges(i)[0];

            std::vector<std::pair<int, double>> nodes {
                {i, dfg.weight(i)},
            };
            std::vector<std::pair<int, int>> edges {
                {pred, i},
                {i, succ},
            };
            output.emplace_back(std::move(nodes), std::move(edges), pred, succ);
        }
    return output;
}
