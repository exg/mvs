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
#include "dfg.h"
#include "intset.h"
#include "nlohmann/json.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stdout, "Usage: config_info CONFIG\n");
        return 1;
    }

    std::unique_ptr<DFG> dfg = DFG::make_dfg(std::cin, false);

    IOSubgraph config(*dfg);
    for (auto &field : split(std::string(argv[1]), ' ')) {
        int v;
        if (!parse_integer(field, v, 0, dfg->num_nodes() - 1)) {
            fprintf(stderr, "invalid configuration\n");
            return 1;
        }
        config.add(v);
    }

    intset closure = config.closure();
    nlohmann::json json = config;
    json["convex"] = config.nodes() == closure;
    json["valid"] = !config.nodes().intersects(dfg->forbidden());
    std::cout << json.dump(4) << std::endl;

    return 0;
}
