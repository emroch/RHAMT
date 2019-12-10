#include "injector.hpp"
#include <cassert>

#define FT 1

bool unit_test(bool (*f)(void), std::string name)
{
    printf("Running %s...  ", name.c_str());
    bool passed = (*f)();
    printf(passed ? "passed\n" : "failed\n");
    return passed;
}

bool test_swap_local(int depth)
{
    // Key, Val, FT, HashType
    Injector<uint16_t, uint16_t, FT, uint16_t> injector;

    for (int i = 0; i < 65536; ++i)
        injector.insert(i, i);

    // Hash, Depth, First, Second
    injector.swap_children_local(0, depth, 0, 1);

    for (int i = 0; i < 65536; ++i) {
        const uint16_t * p = injector.read(i);
        assert(*p == i);
    }

    return 1;
}

bool test_swap_local_shallow(void)
    { return test_swap_local(0); }

bool test_swap_local_deep(void)
    { return test_swap_local(3); }

bool test_swap_other(int depth1, int depth2)
{
    Injector<uint16_t, uint16_t, FT, uint16_t> injector;
    
    for (int i = 0; i < 65536; ++i)
        injector.insert(i, i);

    injector.swap_children_other(0, depth1, -1, depth2, 0);

    for (int i = 0; i < 65536; ++i) {
        const uint16_t * p = injector.read(i);
        assert(*p == i);
    }

    return 1;
}

bool test_swap_other_same(void)
    { return test_swap_other(1, 1); }

bool test_swap_other_diff(void)
    { return test_swap_other(3, 1); }

bool test_set_child(int depth, std::optional<void*> val, int count)
{
    Injector<uint16_t, uint16_t, FT, uint16_t> injector;
    
    for (int i = 0; i < 65536; ++i)
        injector.insert(i, i);

    injector.set_child(0, depth, 0, val, count);

    for (int i = 0; i < 65536; ++i) {
        const uint16_t * p = injector.read(i);
        assert(*p == i);
    }

    return 1;
}

bool test_set_child_null(void)
    { return test_set_child(2, std::optional<void*>(nullptr), FT-1); }

bool test_set_child_rand(void)
    { return test_set_child(2, std::optional<void*>(), FT-1); }

int main(void)
{
    unit_test(test_swap_local_shallow, "test_swap_local_shallow");
    unit_test(test_swap_local_deep, "test_swap_local_deep");

    unit_test(test_swap_other_same, "test_swap_other_same");
    unit_test(test_swap_other_diff, "test_swap_other_diff");

    unit_test(test_set_child_null, "test_set_child_null");
    unit_test(test_set_child_rand, "test_set_child_rand");
    
    return 0;
}
