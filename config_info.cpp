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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stdout, "Usage: config_info CONFIG\n");
        return 1;
    }

    std::unique_ptr<DFG> dfg = DFG::make_dfg(std::cin, false);

    io_config config(*dfg);
    for (auto &field : split(std::string(argv[1]), ' ')) {
        int v;
        if (!parse_integer(field, v, 0, dfg->num_nodes() - 1)) {
            fprintf(stderr, "invalid configuration\n");
            return 1;
        }
        config.add(v);
    }

    fprintf(stdout,
            "CONFIG NUM-INPUTS=%d NUM-OUTPUTS=%d NODES=",
            config.num_in(),
            config.num_out());
    dump_intset(config.nodes(), stdout);
    intset closure = config.closure();
    fprintf(stdout, "CONVEX=%d\n", config.nodes() == closure);
    fprintf(stdout, "VALID=%d\n", !config.nodes().intersects(dfg->forbidden()));

    return 0;
}
