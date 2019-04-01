#include "dfg.h"
#include <cassert>
#include <list>

int main()
{
    DFG dfg = {
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

    std::list<int> order;
    DFSVisitor(dfg, [&order](int u) { order.push_front(u); });
    std::list<int> topo_order {1, 6, 5, 3, 0, 4, 2};
    assert(topo_order == order);
}
