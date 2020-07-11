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

#include "mvs.h"
#include "common.h"
#include "graph.h"
#include "io.h"
#include "vs.h"
#include <cassert>
#include <cmath>

static const bool USE_BK = false;

static bool is_source(const DFG &dfg, const intset &config, int u)
{
    for (auto &v : dfg.in_edges(u)) {
        if (v < dfg.num_nodes() && config.contains(v))
            return false;
    }
    return true;
}

static bool is_sink(const DFG &dfg, const intset &config, int u)
{
    for (auto &v : dfg.out_edges(u)) {
        if (config.contains(v))
            return false;
    }
    return true;
}

int MVSFinder::find_best_recursion_node(int max_num_in,
                                        int max_num_out,
                                        int num_perm_in,
                                        int num_perm_out)
{
    int id = -1;
    std::pair<int, int> best_delta(0, 0);
    for (const auto &u : nodes_left_) {
        bool source = is_source(*dfg_, nodes(), u);
        bool sink = is_sink(*dfg_, nodes(), u);

        if (source || sink) {
            nodes_left_.remove(u);
            std::pair<int, int> delta {
                IOAnalysis::num_perm_in(config_, nodes_left_) - num_perm_in,
                IOAnalysis::num_perm_out(config_, nodes_left_) - num_perm_out,
            };
            nodes_left_.add(u);

            if (max_num_in - num_perm_in > max_num_out - num_perm_out)
                std::swap(delta.first, delta.second);

            if (id == -1 || delta > best_delta) {
                id = u;
                best_delta = delta;
            }
        }
    }
    return id;
}

static double sum_smallest(vmap<int, double> &map, int n)
{
    std::sort(map.begin(),
              map.end(),
              [](const std::pair<int, double> p1,
                 const std::pair<int, double> p2) {
                  return p1.second < p2.second;
              });
    double sum = 0;
    for (int i = 0; i < n; i++)
        sum += map[i].second;
    return sum;
}

void MVSFinder::visit(double dels,
                      bool single,
                      int &max_weight,
                      int max_num_in,
                      int max_num_out)
{
    calls_++;

    if (dels < 0 || (count_ && single))
        return;

    if (config_.num_in() <= max_num_in && config_.num_out() <= max_num_out) {
        double weight = config_.weight();
        int iweight = weight;
        for (auto &cluster : s_clusters_)
            cluster.expand(config_);
        for (auto &cluster : s_nodes_)
            cluster.expand(config_);

        if (single) {
            count_++;
            max_weight = std::max(max_weight, iweight);
        } else if (iweight == max_weight) {
            if (std::find_if(io_output_->begin(),
                             io_output_->end(),
                             [this](const IOSubgraph &mvs) {
                                 return mvs.nodes() == this->nodes();
                             }) == io_output_->end()) {
                count_++;
                io_output_->emplace_back(config_);
            }
        }

        for (auto &cluster : s_clusters_)
            cluster.contract(config_);
        for (auto &cluster : s_nodes_)
            cluster.contract(config_);
        return;
    }

    // pruning
    IOAnalysis analysis(config_, nodes_left_);

    bool prune = false;
    int required_dels_in = 0;
    int required_dels_out = 0;
    if (config_.num_in() - max_num_in > 0) {
        size_t n = config_.num_in() - max_num_in;
        if (analysis.num_perm_in() > max_num_in) {
            if (flags_ & (1 << 1)) {
                pruned_[0]++;
                prune = true;
            }
        } else {
            required_dels_in = ceil(sum_smallest(analysis.get_inputs(), n));
        }
    }
    if (config_.num_out() - max_num_out > 0) {
        if (analysis.num_perm_out() > max_num_out) {
            if (flags_ & (1 << 2)) {
                pruned_[1]++;
                prune = true;
            }
        } else
            required_dels_out = config_.num_out() - max_num_out;
    }

    int num_shared_non_perm_out = std::min({
        analysis.num_shared_non_perm_out(),
        required_dels_in,
        required_dels_out,
    });
    double rnodes_weight = 0;
    if (!prune) {
        rnodes_weight = sum_smallest(analysis.get_rnodes(),
                                     required_dels_in + required_dels_out -
                                         num_shared_non_perm_out);
    }

    if ((flags_ & (1 << 3)) && rnodes_weight > dels) {
        pruned_[2]++;
        prune = true;
    }

    if (prune)
        return;

    int id = find_best_recursion_node(max_num_in,
                                      max_num_out,
                                      analysis.num_perm_in(),
                                      analysis.num_perm_out());
    if (id == -1)
        return;

    nodes_left_.remove(id);

    config_.remove(id);
    visit(dels - dfg_->weight(id), single, max_weight, max_num_in, max_num_out);

    config_.add(id);
    visit(dels, single, max_weight, max_num_in, max_num_out);

    nodes_left_.add(id);
}

int MVSFinder::find_mvsio_(mvs &mvs,
                           bool single,
                           int max_io_weight,
                           int max_num_in,
                           int max_num_out)
{
    nodes_left_ = mvs.nodes();
    nodes_left_.remove(clustered_);
    config_.set(nodes_left_);

    int iweight = ceil(mvs.weight());
    int max_dels = iweight - max_io_weight;
    nlohmann::json json = {
        {"connected", !mvs.disconnected},
        {"enum", !single},
        {"num_inputs", max_num_in},
        {"num_outputs", max_num_out},
        {"num_s-nodes", s_nodes_.size()},
    };
    std::cerr << json.dump() << std::endl;
    if (single) {
        int io_weight = 0;
        switch (itype_) {
            case IterType::LINEAR:
                for (int dels = 1; dels <= max_dels; dels++) {
                    reset_stats();
                    visit(dels, true, io_weight, max_num_in, max_num_out);
                    dump_stats(iweight - dels);
                    if (count_ > 0)
                        break;
                }
                break;
            case IterType::LINEAR_REV:
                for (int dels = max_dels; dels >= 1; dels--) {
                    reset_stats();
                    visit(dels, true, io_weight, max_num_in, max_num_out);
                    dump_stats(iweight - dels);
                    if (count_ == 0)
                        break;
                }
                break;
            case IterType::BINARY_SEARCH:
                int l = 1;
                int r = max_dels;
                while (r >= l) {
                    int dels = (l + r) / 2;
                    reset_stats();
                    visit(dels, true, io_weight, max_num_in, max_num_out);
                    dump_stats(iweight - dels);
                    if (count_ > 0)
                        r = dels - 1;
                    else
                        l = dels + 1;
                }
                break;
        }

        return io_weight;
    } else {
        reset_stats();
        visit(max_dels, false, max_io_weight, max_num_in, max_num_out);
        dump_stats(max_io_weight);
    }

    return max_io_weight;
}

void MVSFinder::find_mvsio(mvs &mvs,
                           bool single,
                           int max_io_weight,
                           int max_num_in,
                           int max_num_out)
{
    if (!(flags_ & (1 << 4))) {
        mvs.io_weight =
            find_mvsio_(mvs, single, max_io_weight, max_num_in, max_num_out);
        return;
    }

    for (auto &cluster : s_clusters_)
        link_cluster(cluster);

    std::vector<double> s_weights;
    int s_node_input_delta = 1;
    if (single || !mvs.disconnected) {
        s_nodes_ = snode_enumerate(mvs, s_clusters_);
        for (auto &cluster : s_nodes_) {
            link_cluster(cluster);
            s_weights.push_back(dfg_->weight(cluster.nodes().front().first));
            if (dfg_->out_edges(cluster.src()).size() > 1)
                s_node_input_delta = 0;
        }
        std::sort(s_weights.begin(), s_weights.end(), std::greater<>());
    }

    mvs.io_weight =
        find_mvsio_(mvs, single, max_io_weight, max_num_in, max_num_out);
    max_io_weight = std::max(max_io_weight, mvs.io_weight);

    if (single && max_num_out > 1 && !s_nodes_.empty()) {
        int m = std::min(max_num_out - 1, (int)s_nodes_.size());
        int _max_num_in = max_num_in;
        int _max_num_out = max_num_out;
        double psum = 0;
        double sum = 0;

        if (s_node_input_delta && m > max_num_in - 1)
            m = max_num_in - 1;
        for (int i = 0; i < m; i++)
            sum += s_weights[i];
        for (int i = 0; i < m; i++) {
            _max_num_in -= s_node_input_delta;
            _max_num_out--;
            psum += s_weights[i];
            int io_weight = find_mvsio_(mvs,
                                        true,
                                        max_io_weight - sum,
                                        _max_num_in,
                                        _max_num_out);
            if (io_weight + psum >= mvs.io_weight) {
                mvs.disconnected = true;
                break;
            }
            if (io_weight + sum < mvs.io_weight)
                break;
        }
    }

    for (auto &cluster : s_nodes_)
        unlink_cluster(cluster);
    s_nodes_.clear();

    if (single && mvs.disconnected)
        mvs.io_weight =
            find_mvsio_(mvs, true, max_io_weight, max_num_in, max_num_out);

    for (auto &cluster : s_clusters_)
        unlink_cluster(cluster);
}

std::vector<IOSubgraph> MVSFinder::enumerate(int max_num_in,
                                             int max_num_out,
                                             IterType itype,
                                             uint8_t flags)
{
    itype_ = itype;
    flags_ = flags;
    nlohmann::json json = {
        {"num_inputs", max_num_in},
        {"num_outputs", max_num_out},
        {"flags", flags_},
    };
    std::cerr << json.dump() << std::endl;

    std::vector<IOSubgraph> output;
    io_output_ = &output;
    int max_io_weight = 0;
    for (auto &mvsc : mvs_vec_) {
        nlohmann::json json = {
            {"enum", false},
            {"max_io_weight", max_io_weight},
            {"mvs", mvsc},
        };
        std::cerr << json.dump() << std::endl;

        int m = flags_ & (1 << 5) ? max_io_weight : 0;
        if (mvsc.weight() >= m) {
            if (mvsc.num_in() > max_num_in || mvsc.num_out() > max_num_out)
                find_mvsio(mvsc, true, m, max_num_in, max_num_out);
            else
                mvsc.io_weight = mvsc.weight();
            max_io_weight = std::max(max_io_weight, mvsc.io_weight);
        }
        json = {
            {"io_weight", mvsc.io_weight},
        };
        std::cerr << json.dump() << std::endl;
    }

    for (auto &mvsc : mvs_vec_) {
        if (mvsc.io_weight == max_io_weight) {
            nlohmann::json json = {
                {"enum", true},
                {"max_io_weight", max_io_weight},
                {"mvs", mvsc},
            };
            std::cerr << json.dump() << std::endl;
            if (mvsc.io_weight < mvsc.weight())
                find_mvsio(mvsc, false, max_io_weight, max_num_in, max_num_out);
            else
                io_output_->emplace_back(mvsc);
        }
    }

    double max_weight = 0;
    for (auto &mvs : output)
        if (mvs.weight() > max_weight && !fp_eq(mvs.weight(), max_weight, 0.01))
            max_weight = mvs.weight();

    for (auto it = output.begin(); it != output.end();)
        if (max_weight > (*it).weight() &&
            !fp_eq((*it).weight(), max_weight, 0.01))
            it = output.erase(it);
        else
            it++;
    return output;
}

void MVSFinder::link_cluster(const SCluster &cluster)
{
    for (auto &edge : cluster.edges())
        dfg_->remove_edge(edge.first, edge.second);
    dfg_->add_edge(cluster.src(), cluster.dst());
    for (auto &node : cluster.nodes()) {
        clustered_.add(node.first);
        dfg_->weight(cluster.dst()) += node.second;
        dfg_->weight(node.first) = 0;
    }
}

void MVSFinder::unlink_cluster(const SCluster &cluster)
{
    dfg_->remove_edge(cluster.src(), cluster.dst());
    for (auto &edge : cluster.edges())
        dfg_->add_edge(edge.first, edge.second);
    for (auto &node : cluster.nodes()) {
        clustered_.remove(node.first);
        dfg_->weight(cluster.dst()) -= node.second;
        dfg_->weight(node.first) = node.second;
    }
}

void MVSFinder::add_config()
{
    mvs_vec_.emplace_back(config_);
}

void MVSFinder::update_config(int id, bool add)
{
    for (auto &v : v_clusters_[id].nodes) {
        if (add)
            config_.add(v);
        else
            config_.remove(v);
    }
}

static void dump_v_graph(const Graph &v_graph,
                         const std::vector<VCluster> &v_clusters)
{
    nlohmann::json json = nlohmann::json::array();
    for (unsigned i = 0; i < v_clusters.size(); i++) {
        json += {
            {"id", i},
            {"nodes", v_clusters[i].nodes},
            {"edges", v_graph.edges(i)},
        };
    };
    std::cerr << json.dump() << std::endl;
}

static void dump_s_clusters(const std::vector<SCluster> &s_clusters)
{
    nlohmann::json json = s_clusters;
    std::cerr << json.dump() << std::endl;
}

MVSFinder::MVSFinder(DFG *dfg)
    : dfg_(dfg)
    , config_(*dfg)
    , nodes_left_(dfg->num_nodes())
    , clustered_(dfg->num_nodes())
{
    // compute P sets and equivalence classes
    auto class_of = std::make_unique<int[]>(dfg->num_nodes());
    intset P(dfg->num_nodes());
    intset F = dfg->forbidden();
    for (int u = 0; u < dfg->num_nodes(); u++) {
        if (F.contains(u))
            continue;
        for (int v = 0; v < dfg->num_nodes(); v++) {
            // check if v is in P(u)
            if (!F.contains(v) && !F.intersects(dfg->pred(u), dfg->succ(v)) &&
                !F.intersects(dfg->succ(u), dfg->pred(v)))
                P.add(v);
        }

        // check if node u belongs to a previously computed equivalence class
        int class_id = -1;
        for (int i = 0; i < v_clusters_.size(); i++) {
            if (P == v_clusters_[i].P()) {
                class_id = i;
                break;
            }
        }

        if (class_id == -1) {
            class_id = v_clusters_.size();
            v_clusters_.emplace_back(P);
        }
        P.clear();
        v_clusters_[class_id].nodes.push_back(u);
        class_of[u] = class_id;
    }

    // build adjacency lists of clusters in the cluster graph
    int num_clusters = v_clusters_.size();
    Graph v_graph(num_clusters);

    for (int i = 0; i < num_clusters; i++) {
        for (const auto &v : v_clusters_[i].P()) {
            if (class_of[v] != i)
                v_graph.add_edge(i, class_of[v]);
        }
    }

    v_graph.invert();

    if (!USE_BK) {
        for (int i = 0; i < dfg->num_nodes(); i++)
            if (!F.contains(i))
                config_.add(i);
    }
    MISFinder finder(
        &v_graph,
        USE_BK,
        [this](const intset &) { this->add_config(); },
        [this](const intset &, int id, bool add) {
            this->update_config(id, add);
        });

    s_clusters_ = scluster_enumerate(*dfg_);

    int n = 0;
    for (auto &cluster : s_clusters_)
        n += cluster.nodes().size();

    nlohmann::json json = {
        {"calls", finder.get_calls()},
        {"num_clusters", num_clusters},
        {"num_mvs-c", finder.get_count()},
        {"num_s-cluster-nodes", n},
    };
    std::cerr << json.dump() << std::endl;

    std::sort(mvs_vec_.begin(),
              mvs_vec_.end(),
              [](const IOSubgraph &i1, const IOSubgraph &i2) {
                  return i1.weight() > i2.weight();
              });
}
