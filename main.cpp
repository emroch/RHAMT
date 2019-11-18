#include "rhamt.hpp"
#include <cassert>

int main(void)
{
    ReliableHAMT<int, int> rhamt;
    for (int i = 0; i < 100; ++i) {
looping:
        rhamt.insert(i, i);
    }
    for (int i = 0; i < 100; ++i) {
looping2:
        assert(i == *rhamt.read(i));
    }

    return 0;
}
