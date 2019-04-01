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
#include "graph.h"
#include <ostream>
#include <unistd.h>

static void find_mis(Graph *graph, bool bk)
{
    auto size = graph->num_nodes();
    double start = get_time();
    MISFinder finder(
        graph,
        bk,
        [size](const intset &name) {},
        [](const intset &name, int id, bool add) {});
    double end = get_time();

    nlohmann::json json = {
        {"calls", finder.get_calls()},
        {"num_MIS", finder.get_count()},
        {"num_edges", graph->num_edges() / 2},
        {"num_nodes", graph->num_nodes()},
        {"time", end - start},
    };
    std::cout << json.dump(4) << std::endl;
}

int main(int argc, char **argv)
{
    bool use_bk = false;
    bool invert = false;
    int c;
    while ((c = getopt(argc, argv, "bi")) != -1) {
        switch (c) {
            case 'b':
                use_bk = true;
                break;
            case 'i':
                invert = true;
                break;
        }
    }

    std::unique_ptr<Graph> graph = Graph::make_graph(std::cin);
    if (invert)
        graph->invert();

    find_mis(graph.get(), use_bk);
}
