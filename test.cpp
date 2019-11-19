#include "rhamt.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <string>

bool unit_test(bool (*f)(void), std::string name) {
    printf("\033[39;1;4mRunning\033[0m \033[96;1;1m%s\033[0m...\n",
            name.c_str());
    bool passed = (*f)();
    if (!passed) {
        printf("\033[96;1;1m%s\033[0m \033[31;1;4mfailed\033[0m\n",
                name.c_str());
        return false;
    }
    printf("\033[96;1;1m%s \033[32;1;4mpassed\033[0m\n", name.c_str());
    return true;
}

bool test_random_sparse(void)
{
    std::unordered_map<int, int> golden;
    ReliableHAMT<int, int> rhamt;
    int k, v;

    for (int i = 0; i < 1000000; ++i) {
        k = rand();
        v = rand();
        golden[k] = v;
        rhamt.insert(k, v);
    }

    if (rhamt.size() != golden.size()) {
        printf("ERROR: %s %d: size mismatch (%lu != %lu)\n", __FILE__, __LINE__,
                        rhamt.size(), golden.size());
        return false;
    }

    for (auto it : golden) {
        const int *rv = rhamt.cread(it.first);
        if (nullptr == rv) {
            printf("ERROR: %s %d: unexpected nullptr\n", __FILE__, __LINE__);
            return false;
        }
        if (*rv != it.second) {
            printf("ERROR: %s %d: unexpected values\n", __FILE__, __LINE__);
            return false;
        }
        rhamt.remove(it.first);
    }

    if (rhamt.size()) {
        printf("ERROR: %s %d: expected size 0, instead size %lu\n",
                __FILE__, __LINE__, rhamt.size());
        return false;
    }

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

bool test_overwrite()
{
    ReliableHAMT<int, int, uint8_t> rhamt;
    bool retval = false;

    for (int i = 0; i < 1024; ++i)
        rhamt.insert(i, i);
    for (int i = 0; i < 1024; ++i)
        rhamt.insert(i, i << 10);

    for (int i = 0; i < 1024; ++i) {
        const int *rv = rhamt.cread(i);
        if (nullptr == rv) {
            printf("ERROR: %s %d: unexpected nullptr\n", __FILE__, __LINE__);
            goto done;
        }
        if ((i << 10) != *rv) {
            printf("ERROR: %s %d: Expected (%d, %d) but found (%d, %d)\n",
                    __FILE__, __LINE__, i, i << 10, i, *rv);
        }
    }

    retval = true;
done:
    return retval;
}

bool test_random_dense()
{
    ReliableHAMT<int, int, uint8_t> rhamt;
    std::unordered_map<int, int> golden;
    bool retval = false;

    for (int i = 0; i < 100000; ++i) {
        int k = rand();
        int v = rand();
        golden[k] = v;
        rhamt.insert(k, v);
    }

    for (auto it : golden) {
        const int * rv = rhamt.cread(it.first);
        if (nullptr == rv) {
            printf("ERROR: %s %d: unexpected nullptr\n", __FILE__, __LINE__);
            goto done;
        }
        if (*rv != it.second) {
            printf("ERROR: %s %d: Expected (%d, %d) but found (%d, %d)\n",
                    __FILE__, __LINE__, it.first, it.second, it.first, *rv);
            goto done;
        }
    }
    retval = true;
done:
    return retval;
}

bool test_string_key()
{
    ReliableHAMT<std::string, int> hamt;
    bool retval = false;

    hamt.insert("Yabadabadoo!", 5132);
    const int * rv = hamt.cread("Yabadabadoo!");
    if (nullptr == rv) {
        printf("ERROR: %s %d: unexpected nullptr\n", __FILE__, __LINE__);
        goto done;
    }
    if (*rv != 5132) {
        printf("ERROR: %s %d: mismatch key-value pair\n", __FILE__, __LINE__);
    }
    retval = true;
done:
    return retval;
}

int main(void)
{
    printf("Beginning Testing...\n");
    srand(time(NULL));

    unit_test(test_small_rhamt, "test_small_rhamt");
    unit_test(test_random_sparse, "test_random_sparse");
    unit_test(test_overwrite, "test_overwrite");
    unit_test(test_random_dense, "test_random_dense");
    unit_test(test_string_key, "test_string_key");

    printf("...Tests Complete\n");

    return 0;
}
