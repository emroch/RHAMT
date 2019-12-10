#ifndef _INJECTOR_HPP
#define _INJECTOR_HPP

#include "rhamt.hpp"

template<class Key, class T, unsigned FT = 0, class HashType = uint32_t,
         class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
class Injector {
    using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
    using Node = RHAMT::Node;
    using SN = RHAMT::SplitNode;
    using LN = RHAMT::LeafNode;

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
    void set_child(const HashType hash, const int depth, unsigned child,
                   std::optional<uintptr_t> val, unsigned count);
    
    // Set the stored hash in the given leaf node to the provided value.
    // If `val` is not set, use a random value. Overwrites the first `count`
    // duplicates.
    void set_hash(const HashType hash, std::optional<HashType> val,
                  unsigned count);

};

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector::swap_children_local(const HashType hash, const int depth,
                              const unsigned first, const unsigned second)
{
    (void)hash;
    (void)depth;
    (void)first;
    (void)second;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector::swap_children_other(const HashType hash1, const int depth1,
                              const HashType hash2, const int depth2,
                              const unsigned child)
{
    (void)hash1;
    (void)depth1;
    (void)hash2;
    (void)depth2;
    (void)child;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector::set_child(const HashType hash, const int depth, unsigned child,
                    std::optional<uintptr_t> val, unsigned count)
{
    (void)hash;
    (void)depth;
    (void)child;
    (void)val;
    (void)count;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
Injector::set_hash(const HashType hash, std::optional<HashType> val,
                   unsigned count)
{
    (void)hash;
    (void)val;
    (void)count;
}


#endif // _INJECTOR_HPP
