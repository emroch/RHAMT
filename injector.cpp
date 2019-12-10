#include "injector.hpp"
#include <cassert>


int main(void)
{
    // Key, Val, FT, HashType
    Injector<uint8_t, uint8_t, 1, uint8_t> injector;

    for (int i = 0; i < 256; ++i)
        injector.insert(i, i);

    // Hash, Depth, First, Second
    injector.swap_children_local(0, 0, 0, 1);

    for (int i = 0; i < 256; ++i) {
        const uint8_t * p = injector.read(i);
        assert(*p == i);
    }

    return 0;
}
