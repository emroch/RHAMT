#include "rhamt.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
#include <cstdio>

bool unit_test(bool (*f)(void), std::string name) {
    printf("Running %s...\n", name.c_str());
    bool passed = (*f)();
    if (!passed) {
        printf("%s failed\n", name.c_str());
        return false;
    }
    printf("%s passed\n\n", name.c_str());
    return true;
}

bool test_small_rhamt()
{
    ReliableHAMT<int, int, uint8_t> rhamt;

    // Fill it up, check that the size matches
    for (int i = 0; i < 256; i++) {
        rhamt.insert(i, i);
    }
    size_t size = rhamt.size();
    if (size != 256) {
        printf("ERROR: size is %ld, expected %d\n", size, 256);
        return false;
    }

    // Remove a few keys
    for (int i = 0; i < 50; i++) {
        rhamt.remove(i);
    }
    size = rhamt.size();
    if (size != 206) {
        printf("ERROR: size is %ld, expected %d\n", size, 206);
        return false;
    }

    // Insert keys bigger than the hash size, forcing collisions
    // Nodes 0-49 will contain single keys, while the rest hold two
    for (int i = 256; i < 512; i++) {
        rhamt.insert(i, i);
    }
    size = rhamt.size();
    if (size != 462) {
        printf("ERROR: size is %ld, expected %d\n", size, 462);
        return false;
    }

    for (int i = 50; i < 512; ++i) {
        int val = *rhamt.read(i);
        if (i != val) {
            printf("ERROR: read %d, expected %d\n", val, i);
            return false;
        }
    }

    return true;
}

int main(void)
{
    printf("Beginning Testing...\n");

    unit_test(test_small_rhamt, "test_small_rhamt");


    return 0;
}
