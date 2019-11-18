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
        static constexpr int nchldrn = 32;
        static constexpr int nlog2chldrn = 5;
        static constexpr HashType subhash(const HashType hash, const int depth)
        {
            return (hash >> (nlog2chldrn * depth)) & ((1 >> nlog2chldrn) - 1);
        }
        static constexpr int soh = sizeof(HashType) * 8;
        static constexpr int maxdepth = soh == 8 ? 2 :
                                        soh == 16 ? 4 :
                                        soh == 32 ? 7 :
                                        soh == 64 ? 13 :
                                        soh == 128 ? 26 : -1;

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
                virtual int remove(HashType, Key, int depth, size_t *) = 0;
        };

        class SplitNode : ReliableHAMT::Node {
            private:
                std::bitset<32> ptrmask;
                std::vector<Node *> children;
                size_t count; // Number of keys in subtree rooted by this
                int getChild(const HashType, const int depth) const;
                //const Node * cgetChild(const HashType, const int depth) const;
            public:
                ~SplitNode();
                int insert(HashType, Key, const T&, int depth);
                const T * cread(HashType, Key, int depth) const;
                T * read(HashType, Key, int depth) const;
                int remove(HashType, Key, int depth, size_t *);
        };

        class LeafNode : ReliableHAMT::Node {
            private:
                std::list<std::pair<Key, T>, Alloc > data;
            public:
                ~LeafNode();
                int insert(HashType, Key, const T&, int depth);
                const T * cread(HashType, Key, int depth) const;
                T * read(HashType, Key, int depth) const;
                int remove(HashType, Key, int depth, size_t *);
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
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::
remove(HashType hash, Key key, int depth, size_t *childcount)
{
    (void)hash;
    (void)depth;
    int rv = 0;
    for (auto & it : data) {
        if (Pred(it.first, key)) {
            data.remove(it);
            rv = 1;
            break;
        }
    }
    *childcount = data.size();
    return rv;
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
//typename ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::Node *
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
getChild(const HashType hash, const int depth) const
{
    HashType shash = ReliableHAMT::subhash(hash, depth);
    if (0 == ptrmask.test(shash)) { // Child is not present
        return -1;
    }
    std::bitset<32> andmask((1 << shash) - 1);
    size_t chldidx = (ptrmask & andmask).count(); // inside [0, 31]
    return chldidx;
}

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
insert(HashType hash, Key key, const T& t, int depth)
{
    using RHAMT = ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>;
    int chldidx = getChild(hash, depth);
    if (-1 == chldidx) {
        Node * newnode;
        if (depth == (maxdepth-1))
            { newnode = new RHAMT::LeafNode(); }
        else
            { newnode = new RHAMT::SplitNode(); }
        children.insert(children.begin() + chldidx, newnode);
    }

    int rv = children[chldidx]->insert(hash, key, t, depth+1);
    count += rv;
    return rv;
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
read(HashType hash, Key key, int depth) const
{
    int chldidx = getChild(hash, depth);
    if (chldidx == -1)
        { return nullptr; }
    return children[chldidx]->read(hash, key, depth+1);
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
cread(HashType hash, Key key, int depth) const
{
    return read(hash, key, depth);
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
remove(HashType hash, Key key, int depth, size_t *childcount)
{
    int chldidx = getChild(hash, depth);
    if (chldidx == -1)
        return 0;
    int rv = children[chldidx]->remove(hash, key, depth+1, childcount);

    count -= rv;

    if (0 == *childcount) {
        delete children[chldidx];
        children.erase(children.begin() + chldidx);
    }
    *childcount = count;
    return rv;
}
#endif
