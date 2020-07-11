#include "mvs.h"
#include <cassert>
#include <fstream>

int main(int argc, char **argv)
{
    if (argc != 5)
        return 1;
    int max_num_in;
    if (!parse_integer(argv[2], max_num_in, 0, INT_MAX))
        return 1;
    int max_num_out;
    if (!parse_integer(argv[3], max_num_out, 0, INT_MAX))
        return 1;
    int output_size;
    if (!parse_integer(argv[4], output_size, 0, INT_MAX))
        return 1;
    std::ifstream input(argv[1]);
    auto dfg = DFG::make_dfg(input, false);
    auto finder = MVSFinder(dfg.get());
    auto itype = MVSFinder::IterType::LINEAR_REV;
    uint8_t flags = 0xff;
    auto output = finder.enumerate(max_num_in, max_num_out, itype, flags);
    assert(output.size() == output_size);
}
