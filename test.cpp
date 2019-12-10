#include "rhamt.hpp"
#include <cassert>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unordered_map>
#include <string>

#define FAIL(msg)   printf("ERROR: %s, %d: %s\n", __FILE__, __LINE__, msg); return false;

#define FT 2

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

bool test_random_sparse()
{
    std::unordered_map<int, int> golden;
    ReliableHAMT<int, int, FT> rhamt;
    int k, v;

    for (int i = 0; i < 100; ++i) {
        k = rand();
        v = rand();
        golden[k] = v;
        rhamt.insert(k, v);
    }

    // if (rhamt.size() != golden.size()) {
    //     printf("ERROR: %s %d: size mismatch (%lu != %lu)\n", __FILE__, __LINE__,
    //                     rhamt.size(), golden.size());
    //     return false;
    // }

    for (auto it : golden) {
        const int *rv = rhamt.read(it.first);
        if (nullptr == rv) {
            FAIL("unexpected nullptr");
        }
        if (*rv != it.second) {
            FAIL("unexpected values");
        }
        rhamt.remove(it.first);
    }

    // if (rhamt.size()) {
    //     printf("ERROR: %s %d: expected size 0, instead size %lu\n",
    //             __FILE__, __LINE__, rhamt.size());
    //     return false;
    // }

    return true;

}
// 
// bool test_small_rhamt()
// {
//     ReliableHAMT<int, int, FT, uint8_t> rhamt;
// 
//     // Fill it up, check that the size matches
//     for (int i = 0; i < 256; i++) {
//         rhamt.insert(i, i);
//     }
//     size_t size = rhamt.size();
//     if (size != 256) {
//         printf("ERROR: %s %d: size is %ld, expected %d\n", __FILE__, __LINE__, size, 256);
//         return false;
//     }
// 
//     // Remove a few keys
//     for (int i = 0; i < 50; i++) {
//         rhamt.remove(i);
//     }
//     size = rhamt.size();
//     if (size != 206) {
//         printf("ERROR: %s %d: size is %ld, expected %d\n", __FILE__, __LINE__, size, 206);
//         return false;
//     }
// 
//     // Insert keys bigger than the hash size, forcing collisions
//     // Nodes 0-49 will contain single keys, while the rest hold two
//     for (int i = 256; i < 512; i++) {
//         rhamt.insert(i, i);
//     }
//     size = rhamt.size();
//     if (size != 462) {
//         printf("ERROR: %s %d: size is %ld, expected %d\n", __FILE__, __LINE__, size, 462);
//         return false;
//     }
// 
//     for (int i = 50; i < 512; ++i) {
//         int val = *rhamt.read(i);
//         if (i != val) {
//             printf("ERROR: %s %d: read %d, expected %d\n", __FILE__, __LINE__, val, i);
//             return false;
//         }
//     }
// 
//     return true;
// }
// 
// bool test_overwrite()
// {
//     ReliableHAMT<int, int, FT, uint8_t> rhamt;
// 
//     for (int i = 0; i < 1024; ++i)
//         rhamt.insert(i, i);
//     for (int i = 0; i < 1024; ++i)
//         rhamt.insert(i, i << 10);
// 
//     for (int i = 0; i < 1024; ++i) {
//         const int *rv = rhamt.read(i);
//         if (nullptr == rv) {
//             FAIL("unexpected nullptr");
//         }
//         if ((i << 10) != *rv) {
//             printf("ERROR: %s %d: Expected (%d, %d) but found (%d, %d)\n",
//                     __FILE__, __LINE__, i, i << 10, i, *rv);
//             return false;
//         }
//     }
// 
//     return true;
// }
// 
// bool test_random_dense()
// {
//     ReliableHAMT<int, int, FT,  uint8_t> rhamt;
//     std::unordered_map<int, int> golden;
// 
//     for (int i = 0; i < 100000; ++i) {
//         int k = rand();
//         int v = rand();
//         golden[k] = v;
//         rhamt.insert(k, v);
//     }
// 
//     for (auto it : golden) {
//         const int * rv = rhamt.read(it.first);
//         if (nullptr == rv) {
//             FAIL("unexpected nullptr");
//         }
//         if (*rv != it.second) {
//             printf("ERROR: %s %d: Expected (%d, %d) but found (%d, %d)\n",
//                     __FILE__, __LINE__, it.first, it.second, it.first, *rv);
//             return false;
//         }
//     }
//     return true;
// }
// 
// bool test_string_key()
// {
//     ReliableHAMT<std::string, int, FT> hamt;
// 
//     hamt.insert("Yabadabadoo!", 5132);
//     const int * rv = hamt.read("Yabadabadoo!");
//     if (nullptr == rv) {
//         FAIL("unexpected nullptr");
//     }
//     if (*rv != 5132) {
//         FAIL("mismatch key-value pair");
//     }
// 
//     return true;
// }
// 
// bool test_missing_read()
// {
//     ReliableHAMT<int, int, FT, uint8_t> hamt;
// 
//     hamt.insert(0, 0);
//     const int * rv = hamt.read(256); // hash collision with key 0
//     if (nullptr != rv) {
//         FAIL("expected nullptr");
//     }
// 
//     rv = hamt.read(1);
//     if (nullptr != rv) {
//         FAIL("expected nullptr");
//     }
// 
//     rv = hamt.read(2);
//     if (nullptr != rv) {
//         FAIL("expected nullptr");
//     }
// 
//     return true;
// }
// 
// bool test_missing_remove()
// {
//     ReliableHAMT<int, int, FT, uint8_t> hamt;
// 
//     hamt.insert(0, 0);
//     int rv = hamt.remove(512);  // remove non-existant key from existing leaf
//     if (0 != rv) {
//         FAIL("removed non-existant key");
//     }
//     if (1 != hamt.size()) {
//         FAIL("lost stored value")
//     }
// 
//     rv = hamt.remove(1);  // remove key from non-existant node
//     if (0 != rv) {
//         FAIL("removed non-existant key");
//     }
//     if (1 != hamt.size()) {
//         FAIL("lost stored value");
//     }
// 
//     if (0 != *hamt.read(0)) {
//         FAIL("incorrect read value");
//     }
// 
//     return true;
// }

int main(void)
{
    printf("Beginning Testing...\n");
    srand(time(NULL));

    // unit_test(test_small_rhamt, "test_small_rhamt");
    unit_test(test_random_sparse, "test_random_sparse");
    // unit_test(test_overwrite, "test_overwrite");
    // unit_test(test_random_dense, "test_random_dense");
    // unit_test(test_string_key, "test_string_key");
    // unit_test(test_missing_read, "test_missing_read");
    // unit_test(test_missing_remove, "test_missing_remove");

    printf("...Tests Complete\n");

    return 0;
}
