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

static const bool VERIFY = false;

// implementation of the algorithm for subgraph enumeration under
// convexity, input and output constraints described in
// https://doi.org/10.1109/CSE.2009.167

static bool verify_config(const DFG &dfg, const io_config &config)
{
    if (config.nodes().intersects(dfg.forbidden()))
        return false;

    return config.nodes() == config.closure();
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
    vs_finder(const DFG &dfg, const config &outputs)
        : dfg_(&dfg)
        , config_(dfg, outputs.closure())
        , F_(config_exclusion(dfg, outputs.nodes()))
    {
    }

    void visit(bool enum_all,
               int max_num_in,
               double &max_weight,
               std::vector<io_config> &result);

private:
    const DFG *dfg_;
    io_config config_;
    intset F_;
};

void vs_finder::visit(bool enum_all,
                      int max_num_in,
                      double &max_weight,
                      std::vector<io_config> &result)
{
    int num_perm_in = 0;
    for (auto &u : config_.inputs()) {
        if (u >= dfg_->num_nodes() || F_.contains(u))
            num_perm_in++;
    }

    if (num_perm_in > max_num_in)
        return;

    int id = -1;
    auto pred = config_.pred();
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
        double weight = config_.weight();

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
    visit(enum_all, max_num_in, max_weight, result);

    config_.remove(id);
    intset F_prev(F_);
    F_.add(id);
    F_.add(dfg_->pred(id));
    visit(enum_all, max_num_in, max_weight, result);
    F_ = F_prev;
}

static void vs_enum_(const DFG &dfg,
                     config &outputs,
                     int size,
                     bool enum_all,
                     int max_num_in,
                     int max_num_out,
                     double &max_weight,
                     std::vector<io_config> &result)
{
    if (size >= 1) {
        vs_finder finder(dfg, outputs);
        finder.visit(enum_all, max_num_in, max_weight, result);
    }
    if (size < max_num_out) {
        auto exclusion = config_exclusion(dfg, outputs.nodes());
        auto pred = outputs.pred();
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

        int min = outputs.nodes().minimum();
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

std::vector<io_config> vs_enum(const DFG &dfg,
                               bool enum_all,
                               int max_num_in,
                               int max_num_out,
                               double &max_weight)
{
    std::vector<io_config> result;
    config outputs(dfg);
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
