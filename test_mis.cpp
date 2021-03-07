#include "graph.h"
#include <cassert>

template <typename T>
static void find_mis(Graph &graph, unsigned count)
{
    auto size = graph.num_nodes();
    T finder(
        &graph,
        [](const intset &name) {},
        [](const intset &name, int id, bool add) {});
    assert(finder.get_count() == count);
}

static void test(Graph &graph, unsigned count)
{
    find_mis<MISFinder>(graph, count);
    find_mis<MISFinderBK>(graph, count);
}

int main()
{
    Graph graph = {
        {0, 4},
        {1, 4},
        {1, 5},
        {1, 6},
        {4, 2},
        {5, 2},
        {5, 3},
        {6, 0},
        {6, 3},
    };

    assert(graph.num_edges() == 18);
    test(graph, 5);
    graph.invert();
    assert(graph.num_edges() == 24);
    test(graph, 9);
    graph.invert();
    assert(graph.num_edges() == 18);
    test(graph, 5);
}
