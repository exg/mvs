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
#include <cstdio>

void vs_io(const DFG &dfg, const intset &nodes, int &num_in, int &num_out)
{
    num_in = 0;
    num_out = 0;

    int id = 0;
    for (;;) {
        bool input;
        int i = io_iter_next(dfg, nodes, id, input);
        if (i == -1)
            break;

        if (input)
            num_in++;
        else
            num_out++;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stdout, "Usage: config_info CONFIG\n");
        return 1;
    }

    std::unique_ptr<DFG> dfg = DFG::make_dfg(std::cin, false);

    intset config(dfg->num_nodes());
    for (auto &field : split(std::string(argv[1]), ' ')) {
        int v;
        if (!parse_integer(field, v, 0, dfg->num_nodes() - 1)) {
            fprintf(stderr, "invalid configuration\n");
            return 1;
        }
        config.add(v);
    }

    int num_in, num_out;
    vs_io(*dfg, config, num_in, num_out);
    fprintf(stdout,
            "CONFIG NUM-INPUTS=%d NUM-OUTPUTS=%d NODES=",
            num_in,
            num_out);
    dump_intset(config, stdout);
    intset closure = config_closure(*dfg, config);
    fprintf(stdout, "CONVEX=%d\n", config == closure);
    fprintf(stdout, "VALID=%d\n", !config.intersects(dfg->forbidden()));

    return 0;
}
