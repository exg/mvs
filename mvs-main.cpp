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

#include "common.h"
#include "mvs.h"
#include <chrono>
#include <climits>
#include <cstdio>
#include <ostream>
#include <unistd.h>

static bool parse_flags(const std::string &str, uint8_t &flags)
{
    flags = 0xff;
    for (auto &field : split(str, ',')) {
        int v;
        if (!parse_integer(field, v, 1, 5))
            return false;
        flags &= ~(uint8_t(1) << v);
    }
    return true;
}

static bool parse_itype(const std::string &str, MVSFinder::IterType &itype)
{
    if (str == "linear")
        itype = MVSFinder::IterType::LINEAR;
    else if (str == "linear-rev")
        itype = MVSFinder::IterType::LINEAR_REV;
    else if (str == "binary-search")
        itype = MVSFinder::IterType::BINARY_SEARCH;
    else
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    MVSFinder::IterType itype = MVSFinder::IterType::LINEAR_REV;
    bool use_weights = false;
    uint8_t flags = 0xff;

    int c;
    while ((c = getopt(argc, argv, "i:o:w")) != -1) {
        switch (c) {
            case 'i':
                if (!parse_itype(std::string(optarg), itype)) {
                    fprintf(stderr, "invalid iteration type\n");
                    return 1;
                }
                break;
            case 'o':
                if (!parse_flags(std::string(optarg), flags)) {
                    fprintf(stderr, "invalid optimization list\n");
                    return 1;
                }
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
                "Usage: mvs [OPTIONS] MAX-IN MAX-OUT\n"
                "  -o ARG\t\tdisable optimizations, ARG is a "
                "comma-separated list of numbers:\n"
                "  \t\t\t  1-3 pruning criterion\n"
                "  \t\t\t  4   clustering\n"
                "  \t\t\t  5   improved weight computation\n"
                "  -i ARG\t\tset iteration type, ARG can be 'linear', "
                "'linear-rev' or  'binary-search'\n"
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

    const auto start = std::chrono::steady_clock::now();
    MVSFinder finder(dfg.get());
    auto output = finder.enumerate(max_num_in, max_num_out, itype, flags);
    const auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = end - start;

    nlohmann::json report = {
        {"max_weight", !output.empty() ? output[0].weight() : 0},
        {"name", dfg->name()},
        {"num_nodes", dfg->num_nodes()},
        {"num_subgraphs", output.size()},
        {"subgraphs", output},
        {"time", elapsed.count()},
    };
    std::cout << report.dump(4) << std::endl;

    return 0;
}
