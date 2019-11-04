#ifndef RHAMT_HPP
#define RHAMT_HPP
#include <vector>
#include <bitset>

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash = hash<Key>,
            class Pred = equal_to<Key>,
            class Alloc = allocator< pair<const Key, T> >
        >
class ReliableHAMT {
    private:
        class Node {
            private:
                HashType subhash;
            public:
                virtual ~Node() {};
                virtual int insert(HashType, const T&, int depth) = 0;
                virtual const T & cread(HashType, int depth) const = 0;
                virtual T & read(HashType, int depth) const = 0;
                virtual int remove(HashType, int depth) = 0;
        };

        class SplitNode : ReliableHAMT::Node {
            private:
                std::bitset<32> ptrmask;
                std::vector<Node *> children;
            public:
                ~Node();
                int insert(HashType, const T&, int depth) = 0;
                const T & cread(HashType, int depth) const = 0;
                T & read(HashType, int depth) const = 0;
                int remove(HashType, int depth) = 0;
        };

        class LeafNode : ReliableHAMT::Node {
            private:
                std::vector<std::pair<Key, T> > data;
            public:
                ~Node();
                int insert(HashType, const T&, int depth) = 0;
                const T & cread(HashType, int depth) const = 0;
                T & read(HashType, int depth) const = 0;
                int remove(HashType, int depth) = 0;
        };

    public:
        int insert(Key, const T&);
        const T & cread(Key) const;
        T & read(Key) const;
        int remove(Key, T);
};

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash = hash<Key>,
            class Pred = equal_to<Key>,
            class Alloc = allocator< pair<const Key, T> >
        >
int
ResilientHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::insert(
        HashType hash, const T&, int depth)
{
    for (auto const & it : 
}

#endif
