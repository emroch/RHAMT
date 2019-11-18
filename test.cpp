#include "rhamt.hpp"
#include <cassert>
#include <iostream>

int main(void)
{
    ReliableHAMT<int, int, uint8_t> rhamt;
    for (int i = 0; i < 256; i++) {
        rhamt.insert(i, i);
    }
    assert(256 == rhamt.size());

    int val = *rhamt.read(127);
    assert(127 == val);

    for (int i = 200; i < 256; i++) {
remove_loop:
        rhamt.remove(i);
    }
    assert(200 == rhamt.size());

    // 256-456 will be hash collisions with 0-200
    for (int i = 256; i < 512; i++) {
        rhamt.insert(i, i);
    }
    assert(456 == rhamt.size());

    for (int i = 0; i < 200; ++i)
        assert(i == *rhamt.read(i));
    for (int i = 256; i < 512; ++i)
        assert(i == *rhamt.read(i));


    return 0;
}
