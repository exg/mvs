#include "common.h"
#include "graph.h"
#include <cassert>
#include <fstream>

static void find_mis(Graph &graph, bool bk, unsigned count)
{
    auto size = graph.num_nodes();
    mis_finder finder(&graph);
    auto stats = finder.visit(bk,
                              [size](const intset &name) {},
                              [](const intset &name, int id, bool add) {});
    assert(stats.first == count);
}

static void test(Graph &graph, unsigned count)
{
    find_mis(graph, false, count);
    find_mis(graph, true, count);
}

int main()
{
    Graph graph(7);
    graph.add_edge(0, 4);
    graph.add_edge(1, 4);
    graph.add_edge(1, 5);
    graph.add_edge(1, 6);
    graph.add_edge(5, 2);
    graph.add_edge(6, 3);
    graph.add_edge(4, 2);
    graph.add_edge(5, 3);
    graph.add_edge(6, 0);

    assert(graph.num_edges() == 18);
    test(graph, 5);
    test(graph, 5);
    graph.invert();
    assert(graph.num_edges() == 24);
    test(graph, 9);
    graph.invert();
    assert(graph.num_edges() == 18);
    test(graph, 5);
}
