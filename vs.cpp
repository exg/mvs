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

static bool verify_config(const DFG &dfg, const IOSubgraph &config)
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

class VSFinder {
public:
    VSFinder(const DFG &dfg, const Subgraph &outputs)
        : dfg_(&dfg)
        , config_(dfg, outputs.closure())
        , F_(config_exclusion(dfg, outputs.nodes()))
    {
    }

    void visit(int max_num_in,
               const std::function<void(const IOSubgraph &)> &output_cb);

private:
    const DFG *dfg_;
    IOSubgraph config_;
    intset F_;
};

void VSFinder::visit(int max_num_in,
                     const std::function<void(const IOSubgraph &)> &output_cb)
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
    for (const auto &u : pred) {
        if (!F_.contains(u))
            id = u;
    }

    if (id == -1) {
        output_cb(config_);

        if (VERIFY)
            assert(verify_config(*dfg_, config_));

        return;
    }

    config_.add(id);
    visit(max_num_in, output_cb);

    config_.remove(id);
    intset F_prev(F_);
    F_.add(id);
    F_.add(dfg_->pred(id));
    visit(max_num_in, output_cb);
    F_ = F_prev;
}

namespace {

void vs_enumerate_(const DFG &dfg,
                   Subgraph &outputs,
                   int size,
                   int max_num_in,
                   int max_num_out,
                   const std::function<void(const IOSubgraph &)> &output_cb)
{
    if (size >= 1) {
        VSFinder finder(dfg, outputs);
        finder.visit(max_num_in, output_cb);
    }
    if (size < max_num_out) {
        auto exclusion = config_exclusion(dfg, outputs.nodes());
        auto pred = outputs.pred();
        intset valid(dfg.num_nodes());
        for (const auto &u : exclusion) {
            if (!dfg.is_forbidden(u) &&
                !(pred.contains(u) && dfg.succ(u).intersects(pred, exclusion)))
                valid.add(u);
        }

        unsigned min = outputs.nodes().minimum();
        for (int u = 0; u < dfg.num_nodes(); u++) {
            if (min != -1 && u >= min)
                break;
            if (valid.contains(u)) {
                outputs.add(u);
                vs_enumerate_(dfg,
                              outputs,
                              size + 1,
                              max_num_in,
                              max_num_out,
                              output_cb);
                outputs.remove(u);
            }
        }
    }
}

}

void vs_enumerate(const DFG &dfg,
                  int max_num_in,
                  int max_num_out,
                  const std::function<void(const IOSubgraph &)> &output_cb)
{
    Subgraph outputs(dfg);
    vs_enumerate_(dfg, outputs, 0, max_num_in, max_num_out, output_cb);
}
