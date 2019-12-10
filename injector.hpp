#ifndef _INJECTOR_HPP
#define _INJECTOR_HPP

#include "rhamt.hpp"
#include <stdexcept>
#include <cstdlib>

template<class Key, class T, unsigned FT, class HashType,
         class Hash, class Pred, class Alloc>
class Injector {
    using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
    using Node = typename RHAMT::Node;
    using SN = typename RHAMT::SplitNode;
    using LN = typename RHAMT::LeafNode;

public:
    RHAMT rhamt;
    
    Injector() {
        srand(time(NULL));
        rhamt = RHAMT();
    }

    // Traverse to `depth` along path given by `hash` and swap the `first`
    // and `second` child pointers (only the first duplicate) to test
    // fast_traverse's ability to detect incorrect leaf node
    void swap_children_local(const HashType hash, const int depth,
                             const unsigned first, const unsigned second);
    
    // Traverse to `depth1` along path `hash1` and to `depth2` along path
    // `hash2` and swap the `child`th pointer (first duplicate only) to test
    // more severe structural mistakes (i.e. skipping levels)
    void swap_children_other(const HashType hash1, const int depth1,
                             const HashType hash2, const int depth2,
                             unsigned child);

    // Set the selected child pointer to the given value. If `val` is not
    // specified, a random value is assinged. The first `count` duplicates
    // will be overwritten
    void set_child(const HashType hash, const int depth,
                   unsigned child, std::optional<uintptr_t> val,
                   unsigned count);
    
    // Set the stored hash in the given leaf node to the provided value.
    // If `val` is not set, use a random value. Overwrites the first `count`
    // duplicates.
    void set_hash(const HashType hash,
                  std::optional<HashType> val, unsigned count);

    const T * insert(const Key& key, const T& val) {
        return rhamt.insert(key, val);
    }
    int remove(const Key& key) {
        return rhamt.remove(key);
    }
    const T * read(const Key& key) {
        return rhamt.read(key);
    }

};



template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector<Key, T, FT, HashType, Hash, Pred, Alloc>::
swap_children_local(const HashType hash, const int depth,
                              const unsigned first, const unsigned second)
{
    if (first >= RHAMT::nchldrn || second >= RHAMT::nchldrn)
        throw std::out_of_range("Child index must be < 32");
    if (depth >= RHAMT::maxdepth)
        throw std::out_of_range("Depth must be < maxdepth");

    int level = 0;
    SN* curr_node = &rhamt._root;
    while (level < depth) {
        HashType shash = RHAMT::subhash(hash, level);
        curr_node = reinterpret_cast<SN*>(curr_node->children[shash][0]);
    }

    std::swap(curr_node->children[first][0], curr_node->children[second][0]);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector<Key, T, FT, HashType, Hash, Pred, Alloc>::
swap_children_other(const HashType hash1, const int depth1,
                              const HashType hash2, const int depth2,
                              const unsigned child)
{
    (void)hash1;
    (void)hash2;

    if (child >= RHAMT::nchldrn)
        throw std::out_of_range("Child index must be < 32");
    if (depth1 >= RHAMT::maxdepth || depth2 >= RHAMT::maxdepth)
        throw std::out_of_range("Depth must be < maxdepth");

    int level = 0;
    Node* curr_node_a = rhamt._root;
    while (level < depth1) {
        HashType shash = RHAMT::subhash(hash1, level);
        curr_node_a = curr_node_a->children[shash][0];
    }

    level = 0;
    Node* curr_node_b = rhamt._root;
    while (level < depth2) {
        HashType shash = RHAMT::subhash(hash2, level);
        curr_node_b = curr_node_b->children[shash][0];
    } 

    std::swap(curr_node_a->children[child][0], curr_node_b->children[child][0]);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector<Key, T, FT, HashType, Hash, Pred, Alloc>::
set_child(const HashType hash, const int depth, unsigned child,
                    std::optional<uintptr_t> val, unsigned count)
{
    if (child >= RHAMT::nchldrn)
        throw std::out_of_range("Child index must be < 32");
    if (depth >= RHAMT::maxdepth)
        throw std::out_of_range("Depth must be < maxdepth");

    int level = 0;
    Node* curr_node = rhamt._root;
    while (level < depth) {
        HashType shash = RHAMT::subhash(hash, level);
        curr_node = curr_node->children[shash][0];
    }

    Node* rand_ptr = (Node*)rand();
    for (int i = 0; i < count; ++i)
        curr_node->children[child][i] = val.value_or(rand_ptr);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector<Key, T, FT, HashType, Hash, Pred, Alloc>::
set_hash(const HashType hash, std::optional<HashType> val,
                   unsigned count)
{
    int level = 0;
    Node* curr_node = rhamt._root;
    while (level < RHAMT::maxdepth) {
        HashType shash = RHAMT::subhash(hash, level);
        curr_node = curr_node->children[shash][0];
    }

    HashType rand_hash = (HashType)rand();
    for (int i = 0; i < count; ++i)
        curr_node->hash[i] = val.value_or(rand_hash);
}


#endif // _INJECTOR_HPP
