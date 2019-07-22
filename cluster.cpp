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
#include "vs.h"

static bool is_maximal(const std::vector<intset> &vs, int i)
{
    for (int j = 0; j < vs.size(); j++) {
        if (j != i && vs[i].is_subset_of(vs[j]))
            return false;
    }
    return true;
}

std::vector<s_cluster> scluster_enum(const DFG &dfg)
{
    double max_weight;
    std::vector<s_cluster> output;
    auto vs = vs_enum(dfg, true, 1, 1, max_weight);

    for (int i = 0; i < vs.size(); i++) {
        if (vs[i].size() > 1 && is_maximal(vs, i)) {
            std::vector<int> nodes;
            std::vector<std::pair<int, int>> edges;
            int vi = -1;
            int vo = -1;
            int u = 0;
            for (;;) {
                u = vs[i].find_next(u);
                if (u == -1)
                    break;

                for (auto &v : dfg.out_edges(u)) {
                    if (vs[i].contains(v))
                        edges.emplace_back(u, v);
                    else
                        vo = u;
                }

                if (u != vo)
                    nodes.push_back(u);

                for (auto &v : dfg.in_edges(u)) {
                    if (v >= dfg.num_nodes() || !vs[i].contains(v)) {
                        edges.emplace_back(v, u);
                        vi = v;
                    }
                }
                u++;
            }

            output.emplace_back(std::move(nodes), std::move(edges), vi, vo);
        }
    }
    return output;
}

std::vector<s_cluster> snode_enum(const DFG &dfg,
                                  const intset &config,
                                  const std::vector<s_cluster> &s_clusters)
{
    intset cluster_roots(dfg.num_nodes());
    for (auto &cluster : s_clusters)
        cluster_roots.add(cluster.dst());
    std::vector<s_cluster> output;
    for (int i = 0; i < dfg.num_nodes(); i++)
        if (!cluster_roots.contains(i) && config.contains(i) &&
            dfg.in_edges(i).size() == 1 && dfg.out_edges(i).size() == 1 &&
            !config.contains(dfg.in_edges(i)[0]) &&
            config.contains(dfg.out_edges(i)[0])) {
            int pred = dfg.in_edges(i)[0];
            int succ = dfg.out_edges(i)[0];

            std::vector<int> nodes { i };
            std::vector<std::pair<int, int>> edges {
                { pred, i },
                { i, succ },
            };
            output.emplace_back(std::move(nodes), std::move(edges), pred, succ);
        }
    return output;
}
