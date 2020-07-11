/* Copyright (C) 2013-2020 Emanuele Giaquinta

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

#include "io.h"

static bool is_permanent(const DFG &dfg,
                         const intset &config,
                         const intset &nodes_left,
                         int u)
{
    if (!nodes_left.contains(u))
        return true;
    if (!dfg.pred(u).intersects_difference(config, nodes_left))
        return false;
    if (!dfg.succ(u).intersects_difference(config, nodes_left))
        return false;
    return true;
}

static bool input_is_permanent(const DFG &dfg,
                               const intset &config,
                               const intset &nodes_left,
                               int u)
{
    for (auto &v : dfg.out_edges(u)) {
        if (config.contains(v) && is_permanent(dfg, config, nodes_left, v))
            return true;
    }
    return false;
}

IOAnalysis::IOAnalysis(const IOSubgraph &config, const intset &nodes_left)
{
    for (auto &v : config.inputs()) {
        if (input_is_permanent(config.dfg(), config.nodes(), nodes_left, v)) {
            num_perm_in_++;
        } else {
            inputs_.add(v, 0);
            for (auto &z : config.dfg().out_edges(v)) {
                if (config.nodes().contains(z)) {
                    double &value = rnodes_.add(z, 0);
                    value++;
                }
            }
        }
    }

    for (auto &input : inputs_) {
        int v = input.first;
        for (auto &z : config.dfg().out_edges(v)) {
            if (config.nodes().contains(z)) {
                double &value = rnodes_.add(z, 0);
#if 1
                input.second += 1. / value;
#else
                if (value == 1)
                    input.second++;
#endif
            }
        }
    }

    for (auto &output : config.outputs())
        if (is_permanent(config.dfg(), config.nodes(), nodes_left, output))
            num_perm_out_++;
        else {
            double &value = rnodes_.add(output, 0);
            if (value >= 1)
                num_shared_non_perm_out_++;
        }

    for (auto &entry : rnodes_)
        entry.second = config.dfg().weight(entry.first);
}

int IOAnalysis::num_perm_in(const IOSubgraph &config, const intset &nodes_left)
{
    int n = 0;
    for (auto &v : config.inputs())
        if (input_is_permanent(config.dfg(), config.nodes(), nodes_left, v))
            n++;
    return n;
}

int IOAnalysis::num_perm_out(const IOSubgraph &config, const intset &nodes_left)
{
    int n = 0;
    for (auto &v : config.outputs())
        if (is_permanent(config.dfg(), config.nodes(), nodes_left, v))
            n++;
    return n;
}
