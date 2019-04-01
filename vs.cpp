/* Copyright (C) 2014-2019 Emanuele Giaquinta

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

#include "vs.h"
#include <stdio.h>

static const bool VERIFY = false;

// implementation of the algorithm for subgraph enumeration under
// convexity, input and output constraints described in
// https://doi.org/10.1109/CSE.2009.167

static intset config_pred(const DFG &dfg, const intset &config)
{
    intset out(dfg.num_nodes());
    int u = 0;
    for (;;) {
        u = config.find_next(u);
        if (u == -1)
            break;

        out.add_difference(dfg.pred(u), config);
        u++;
    }

    return out;
}

static intset config_succ(const DFG &dfg, const intset &config)
{
    intset out(dfg.num_nodes());
    int u = 0;
    for (;;) {
        u = config.find_next(u);
        if (u == -1)
            break;

        out.add_difference(dfg.succ(u), config);
        u++;
    }

    return out;
}

intset config_closure(const DFG &dfg, const intset &config)
{
    auto out(config);
    auto pred = config_pred(dfg, config);
    auto succ = config_succ(dfg, config);

    out.add_intersection(pred, succ);

    return out;
}

static void vs_update_io(const DFG &dfg,
                         const intset &nodes,
                         int u,
                         bool add,
                         vset<int> &inputs)
{
    io_delta_iter iter(dfg, nodes, u, add);
    for (;;) {
        bool input;
        bool add;
        int v = iter.next(input, add);
        if (v == -1)
            break;

        if (input) {
            if (add)
                inputs.add(v);
            else
                inputs.remove(v);
        }
    }
}

static vset<int> vs_io(const DFG &dfg, const intset &nodes)
{
    vset<int> inputs;
    int id = 0;
    for (;;) {
        bool input;
        int i = io_iter_next(dfg, nodes, id, input);
        if (i == -1)
            break;

        if (input)
            inputs.add(i);
    }
    return inputs;
}

static bool verify_config(const DFG &dfg, const intset &config)
{
    if (config.intersects(dfg.forbidden()))
        return false;

    return config == config_closure(dfg, config);
}

static intset config_exclusion(const DFG &dfg, const intset &config)
{
    intset out(dfg.forbidden());

    for (int b = dfg.num_nodes() - 1; b >= 0; b--)
        if (out.contains(b))
            for (auto &a : dfg.in_edges(b)) {
                if (a < dfg.num_nodes() && !config.contains(a))
                    out.add(a);
            }

    return out;
}

class vs_finder {
public:
    vs_finder(const DFG &dfg, const intset &outputs)
        : dfg_(&dfg)
        , config_(config_closure(dfg, outputs))
        , F_(config_exclusion(dfg, outputs))
        , inputs_(vs_io(dfg, config_))
    {
    }

    void visit(bool enum_all,
               int max_num_in,
               double &max_weight,
               std::vector<intset> &result);

private:
    const DFG *dfg_;
    intset config_;
    intset F_;
    vset<int> inputs_;
};

void vs_finder::visit(bool enum_all,
                      int max_num_in,
                      double &max_weight,
                      std::vector<intset> &result)
{
    int num_perm_in = 0;
    for (int u : inputs_) {
        if (u >= dfg_->num_nodes() || F_.contains(u))
            num_perm_in++;
    }

    if (num_perm_in > max_num_in)
        return;

    int id = -1;
    auto pred = config_pred(*dfg_, config_);
    int u = 0;
    for (;;) {
        u = pred.find_next(u);
        if (u == -1)
            break;

        if (!F_.contains(u))
            id = u;
        u++;
    }

    if (id == -1) {
        double weight = config_weight(*dfg_, config_);

        if (enum_all || weight >= max_weight) {
            if (!fp_eq(weight, max_weight, 0.01)) {
                max_weight = weight;
                if (!enum_all)
                    result.clear();
            }
            result.emplace_back(config_);
        }

        if (VERIFY)
            assert(verify_config(*dfg_, config_));

        return;
    }

    config_.add(id);
    vs_update_io(*dfg_, config_, id, true, inputs_);
    visit(enum_all, max_num_in, max_weight, result);

    config_.remove(id);
    vs_update_io(*dfg_, config_, id, false, inputs_);
    intset F_prev(F_);
    F_.add(id);
    F_.add(dfg_->pred(id));
    visit(enum_all, max_num_in, max_weight, result);
    F_ = F_prev;
}

static void vs_enum_(const DFG &dfg,
                     intset &outputs,
                     int size,
                     bool enum_all,
                     int max_num_in,
                     int max_num_out,
                     double &max_weight,
                     std::vector<intset> &result)
{
    if (size >= 1) {
        vs_finder finder(dfg, outputs);
        finder.visit(enum_all, max_num_in, max_weight, result);
    }
    if (size < max_num_out) {
        auto exclusion = config_exclusion(dfg, outputs);
        auto pred = config_pred(dfg, outputs);
        intset valid(dfg.num_nodes());
        int u = 0;
        for (;;) {
            u = exclusion.find_next(u);
            if (u == -1)
                break;

            if (!dfg.forbidden().contains(u) &&
                !(pred.contains(u) && dfg.succ(u).intersects(pred, exclusion)))
                valid.add(u);
            u++;
        }

        int min = outputs.minimum();
        for (int u = 0; u < dfg.num_nodes(); u++) {
            if (min != -1 && u >= min)
                break;
            if (valid.contains(u)) {
                outputs.add(u);
                vs_enum_(dfg,
                         outputs,
                         size + 1,
                         enum_all,
                         max_num_in,
                         max_num_out,
                         max_weight,
                         result);
                outputs.remove(u);
            }
        }
    }
}

std::vector<intset> vs_enum(const DFG &dfg,
                            bool enum_all,
                            int max_num_in,
                            int max_num_out,
                            double &max_weight)
{
    std::vector<intset> result;
    intset outputs(dfg.num_nodes());
    max_weight = 0;
    vs_enum_(dfg,
             outputs,
             0,
             enum_all,
             max_num_in,
             max_num_out,
             max_weight,
             result);
    return result;
}
