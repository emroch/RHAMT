#ifndef RHAMT_HPP
#define RHAMT_HPP
#include <vector>
#include <list>
#include <bitset>
#include <cstdint>
#include <functional>

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash = std::hash<Key>,
            class Pred = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
class ReliableHAMT {
    private:
        class Node {
            private:
                HashType subhash;
            public:
                virtual ~Node() {};
                /*
                 *  returns 1 if a new key is inserted, otherwise 0 for update
                 */
                virtual int insert(HashType, Key, const T&, int depth) = 0;
                virtual const T * cread(HashType, Key, int depth) const = 0;
                virtual T * read(HashType, Key, int depth) const = 0;
                virtual int remove(HashType, Key, int depth) = 0;
        };

        class SplitNode : ReliableHAMT::Node {
            private:
                std::bitset<32> ptrmask;
                std::vector<Node *> children;
            public:
                ~SplitNode();
                int insert(HashType, Key, const T&, int depth);
                const T * cread(HashType, Key, int depth) const;
                T * read(HashType, Key, int depth) const;
                int remove(HashType, Key, int depth);
        };

        class LeafNode : ReliableHAMT::Node {
            private:
                std::list<std::pair<Key, T>, Alloc > data;
            public:
                ~LeafNode();
                int insert(HashType, Key, const T&, int depth);
                const T * cread(HashType, Key, int depth) const;
                T * read(HashType, Key, int depth) const;
                int remove(HashType, Key, int depth);
        };

    public:
        int insert(Key, const T&);
        const T & cread(Key) const;
        T & read(Key) const;
        int remove(Key);
};

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::insert(
        HashType hash, Key key, const T& t, int depth)
{
    for (auto & it : data) {
        if (Pred(it.first, key)) {
            it.second = t;
            return 0;
        }
    }
    data.push_back(make_pair(key, t));
    return 1;
}

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::read(
        HashType hash, Key key, int depth) const
{
    (void)hash;
    (void)depth;
    for (auto const & it : data) {
        if (Pred(it.first, key)) {
            return &it.second;
        }
    }
    return nullptr;
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::cread(
        HashType hash, Key key, int depth) const
{
    return this->read(hash, key, depth);
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::remove(
        HashType hash, Key key, int depth)
{
    (void)hash;
    (void)depth;
    for (auto & it : data) {
        if (Pred(it.first, key)) {
            data.remove(it);
            return 1;
        }
    }
    return 0;
}
#endif
