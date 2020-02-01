# Reliable Hash Array Mapped Trie (RHAMT)

Hash array mapped tries (HAMT) are a common implementation of an associative
array. Key-value pairs are stored in the trie by first hashing the keys to
ensure an even distribution and consistent key length. The hashed keys are then
used to traverse the trie and the value is stored at the leaf.

In the presence of memory errors, the tree-based structure of a HAMT presents
many opportunities for failure. An incorrectly hashed value may cause traversal
to a distant part of the trie, or a corrupted pointer may cause a segmentation
fault. Even worse, a corrupted pointer may point to valid, but incorrect,
memory. To prevent memory errors from affecting the data store, we implement
a reliable HAMT, which uses redundant data and a voting mechanism to ensure we
arrive in the correct location.

Each pointer in the trie is stored as `2F+1` duplicates, where `F` is the number
of faults to tolerate (see [Error Model](#error-model) for more details). During
normal operation, the trie is traversed as usual, following each pointer to the
next node. In the event of a segmentation fault, traversal begins again at the
root of the trie, this time voting on each pointer and correcting any mistakes
before continuing. Similarly, if the hash of the key data found in the leaf does
not match the hash used to reach the leaf, then the trie enters repair mode and
each pointer along the path is voted on and corrected.

## Compiling

This code requires features introduced in C++17. Be sure to use the `-std=c++17`
compilation flag to enable these features. It is also recommended to use g++
version 9.2 or higher, as that is how we tested.

To run the provided test suite (timing and correctness testing), first enable
the desired tests by modifying `test.cpp`, then run
```bash
$ g++ -std=c++17 test.cpp -o test.out
$ ./test.out
```

To run the fault injection campaign, run
```bash
$ g++ -std=c++17 injector.cpp -o injector.out
$ ./injector.out
```

## Guidelines

1. For `std::allocator` only use `allocate` and `deallocate` member functions
as all other member functions are deprecated as of C++20.

## Error Model

Faulty-RAM error model. We assume that our system memory may develop faults,
both transient and permanent, which manifest as incorrect bits (either flipped,
as from radiation, or stuck, as from short circuits).

Our fault tolerance guarantee holds under the assumption of fewer than `F` faulty
*data elements*, such as pointers, integers, or key values. Multiple bit errors
in the same data element are considered a single error, making the actual
fault tolerance of the data structure much higher in the average case.

### Safety

We consider instruction memory, as well as "voting" memory to be hardware
fault-tolerant. This may be through mechanisms such as ECC memory, or any other
system to ensure correctness. Without this assumption, an error in the voting
mechanism or instruction memory may cause incorrect program output regardless of
the reliability of our data structure.
