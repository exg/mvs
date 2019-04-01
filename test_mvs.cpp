#include "mvs.h"
#include <cassert>
#include <fstream>

static void test_crypt(const char *path)
{
    std::ifstream input(path);
    auto dfg = DFG::make_dfg(input, false);
    auto finder = MVSFinder(dfg.get());
    auto itype = MVSFinder::IterType::LINEAR_REV;
    uint8_t flags = 0xff;
    auto output = finder.enumerate(1, 1, itype, flags);
    assert(output.size() == 64);
    output = finder.enumerate(2, 2, itype, flags);
    assert(output.size() == 14);
}

static void test_hadamard(const char *path)
{
    std::ifstream input(path);
    auto dfg = DFG::make_dfg(input, false);
    auto finder = MVSFinder(dfg.get());
    auto itype = MVSFinder::IterType::LINEAR_REV;
    uint8_t flags = 0xff;
    auto output = finder.enumerate(18, 18, itype, flags);
    assert(output.size() == 1);
    output = finder.enumerate(17, 17, itype, flags);
    assert(output.size() == 1);
    output = finder.enumerate(16, 16, itype, flags);
    assert(output.size() == 1);
    output = finder.enumerate(15, 15, itype, flags);
    assert(output.size() == 16);
    output = finder.enumerate(14, 14, itype, flags);
    assert(output.size() == 8);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    test_crypt(argv[1]);
    test_hadamard(argv[2]);
}
