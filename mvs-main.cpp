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
#include <climits>
#include <cstdio>
#include <unistd.h>

static void print_mvsio(const io_config &mvs)
{
    fprintf(stdout,
            "MVS-CIO NUM-INPUTS=%d NUM-OUTPUTS=%d NODES=",
            mvs.num_in(),
            mvs.num_out());
    dump_intset(mvs.nodes(), stdout);
}

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

static bool parse_itype(const std::string &str, iter_type &itype)
{
    if (str == "linear")
        itype = iter_type::LINEAR;
    else if (str == "linear-rev")
        itype = iter_type::LINEAR_REV;
    else if (str == "binary-search")
        itype = iter_type::BINARY_SEARCH;
    else
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    iter_type itype = iter_type::LINEAR_REV;
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

    fprintf(stdout,
            "DFG NAME=%s NUM-NODES=%d\n",
            dfg->name().c_str(),
            dfg->num_nodes());
    fflush(stdout);

    double start = get_time();
    mvs_finder finder(dfg.get());
    auto output = finder.mvs_enum(max_num_in, max_num_out, itype, flags);
    double end = get_time();

    fprintf(stdout, "\n");
    for (auto &mvs : output) {
        print_mvsio(mvs);
        fprintf(stdout, "\n");
    }
    fprintf(stdout,
            "COUNT=%lu MAX-WEIGHT=%.2f TIME=%.2f\n",
            output.size(),
            !output.empty() ? output[0].weight() : 0,
            end - start);

    return 0;
}
