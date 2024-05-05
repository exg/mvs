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

#include "common.h"
#include "vs.h"
#include <chrono>
#include <climits>
#include <cstdio>
#include <ostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    bool enum_all = false;
    bool use_weights = false;

    int c;
    while ((c = getopt(argc, argv, "ew:")) != -1) {
        switch (c) {
            case 'e':
                enum_all = true;
                break;
            case 'w':
                use_weights = true;
                break;
        }
    }
    argc -= optind - 1;
    argv += optind - 1;

    if (argc < 3) {
        fprintf(stdout,
                "Usage: vs [OPTIONS] MAX-IN MAX-OUT\n"
                "  -e\t\t\tenumerate all\n"
                "  -w\t\t\tuse real weights\n");
        return 1;
    }

    int max_num_in;
    if (!parse_integer(std::string(argv[1]), max_num_in, 0, INT_MAX)) {
        fprintf(stderr, "invalid input threshold\n");
        return 1;
    }
    int max_num_out;
    if (!parse_integer(std::string(argv[2]), max_num_out, 0, INT_MAX)) {
        fprintf(stderr, "invalid output threshold\n");
        return 1;
    }

    std::unique_ptr<DFG> dfg = DFG::make_dfg(std::cin, use_weights);

    if (dfg->forbidden().size() == dfg->num_nodes())
        return 1;

    double max_weight = 0;
    const auto start = std::chrono::steady_clock::now();
    std::vector<IOSubgraph> output;
    vs_enumerate(*dfg,
                 max_num_in,
                 max_num_out,
                 [&max_weight, &output, enum_all](const IOSubgraph &subgraph) {
                     double weight = subgraph.weight();
                     if (enum_all || weight >= max_weight) {
                         if (!fp_eq(weight, max_weight, 0.01)) {
                             max_weight = weight;
                             if (!enum_all)
                                 output.clear();
                         }
                     }
                     output.emplace_back(subgraph);
                 });
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    nlohmann::json report = {
        {"max_weight", max_weight},
        {"name", dfg->name()},
        {"num_nodes", dfg->num_nodes()},
        {"num_subgraphs", output.size()},
        {"subgraphs", output},
        {"time", elapsed.count()},
    };
    std::cout << report.dump(4) << std::endl;

    return 0;
}
