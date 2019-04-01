#include "common.h"
#include "mvs.h"
#include <cassert>
#include <fstream>

static void test_crypt(const char *path)
{
    std::ifstream input(path);
    auto dfg = DFG::make_dfg(input, false);
    auto finder = mvs_finder(dfg.get());
    auto itype = iter_type::LINEAR_REV;
    uint8_t flags = 0xff;
    auto output = finder.mvs_enum(1, 1, itype, flags);
    assert(output.size() == 64);
    output = finder.mvs_enum(2, 2, itype, flags);
    assert(output.size() == 14);
}

static void test_hadamard(const char *path)
{
    std::ifstream input(path);
    auto dfg = DFG::make_dfg(input, false);
    auto finder = mvs_finder(dfg.get());
    auto itype = iter_type::LINEAR_REV;
    uint8_t flags = 0xff;
    auto output = finder.mvs_enum(18, 18, itype, flags);
    assert(output.size() == 1);
    output = finder.mvs_enum(17, 17, itype, flags);
    assert(output.size() == 1);
    output = finder.mvs_enum(16, 16, itype, flags);
    assert(output.size() == 1);
    output = finder.mvs_enum(15, 15, itype, flags);
    assert(output.size() == 16);
    output = finder.mvs_enum(14, 14, itype, flags);
    assert(output.size() == 8);
}

int main(int argc, char **argv)
{
    test_crypt(argv[1]);
    test_hadamard(argv[2]);
}
