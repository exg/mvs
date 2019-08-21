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
#include <climits>
#include <cstdio>
#include <unistd.h>

static void print_mvsio(const io_config &config)
{
    fprintf(stdout,
            "MVS-CIO NUM-INPUTS=%d NUM-OUTPUTS=%d NODES=",
            config.num_in(),
            config.num_out());
    dump_intset(config.nodes(), stdout);
}

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

    fprintf(stdout,
            "DFG NAME=%s NUM-NODES=%d\n",
            dfg->name().c_str(),
            dfg->num_nodes());
    fflush(stdout);

    double max_weight;
    double start = get_time();
    auto output = vs_enum(*dfg, enum_all, max_num_in, max_num_out, max_weight);
    double end = get_time();

    fprintf(stdout, "\n");
    for (auto &config : output) {
        print_mvsio(config);
        fprintf(stdout, "\n");
    }
    fprintf(stdout,
            "COUNT=%lu MAX-WEIGHT=%.2f TIME=%.2f\n",
            output.size(),
            max_weight,
            end - start);

    return 0;
}
