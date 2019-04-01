#include "intset.h"
#include <cassert>

enum { N = 128 };

static bool elements[256];

static void read_data()
{
    FILE *f = fopen("/dev/urandom", "r");
    for (int i = 0; i < N; i++) {
        uint8_t elem;
        int count = fread(&elem, 1, 1, f);
        assert(count == 1);
        elements[elem] = true;
    }
}

int main()
{
    read_data();
    intset s(256);
    assert(s.minimum() == -1);
    unsigned i;
    for (i = 0; i < 256; i++)
        if (elements[i])
            s.add(i);
    for (i = 0; i < 256; i++) {
        bool has = s.contains(i);
        assert(has == elements[i]);
    }
    for (const auto &elem : s) {
        assert(elements[elem]);
    }
    for (i = 0; i < 256; i++)
        s.remove(i);
    assert(s.minimum() == -1);
}
