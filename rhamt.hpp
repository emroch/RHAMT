#ifndef _RHAMT_HPP
#define _RHAMT_HPP
#include <vector>
#include <list>
#include <bitset>
#include <cstdint>
#include <functional>

// #define template_def template< class Key,                          /* type of key   */  \
//                                class T,                            /* type of value */  \
//                                class HashType  = uint32_t,         /* size of hash  */  \
//                                class Hash = std::hash<Key>,        /* hash function */  \
//                                class Pred = std::equal_to<Key>,    /*               */  \
//                                class Alloc = std::allocator< std::pair<const Key, T> >  \
//                               >

template<   class Key,                          // type of key
            class T,                            // type of value
            class HashType  = uint32_t,         // size of hash
            class Hash = std::hash<Key>,        // hash function
            class Pred = std::equal_to<Key>,    //
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
class ReliableHAMT {
    private:
        /* Number of children for each node */
        static constexpr int nchldrn = 32;
        /* Size of index for child node array */
        static constexpr int nlog2chldrn = 5;
        /* Extract subhash for indexing child array at depth */
        static constexpr HashType subhash(const HashType hash, const int depth)
        {
            // Shift the full hash by `nlog2chldrn` for each level of the tree,
            // then mask the lowest `nlog2chldrn` bits to get the subhash
            return (hash >> (nlog2chldrn * depth)) & ((1 << nlog2chldrn) - 1);
        }
        /* Max depth of tree, assuming 5 bits of key consumed at each level */
        static constexpr int soh = sizeof(HashType) * 8;
        static constexpr int maxdepth = soh == 8 ? 2 :
                                        soh == 16 ? 4 :
                                        soh == 32 ? 7 :
                                        soh == 64 ? 13 :
                                        soh == 128 ? 26 : -1;

        /* Virtual base class */
        class Node {
            // private:
            //     HashType subhash;
            public:
                virtual ~Node() {};
                /* Returns 1 if a new key is inserted or 0 if key is updated */
                virtual int insert(HashType, Key, const T&, int depth) = 0;
                /* Retrieve a value from HAMT with given hash and key */
                virtual T * read(HashType, Key, int depth) = 0;
                virtual const T * cread(HashType, Key, int depth) = 0;
                /* Remove a value from HAMT with given hash and key, returning
                 * 1 if a leaf is destroyed, otherwise 0
                 */
                virtual int remove(HashType, Key, int depth, size_t *) = 0;
        };

        class SplitNode : public ReliableHAMT::Node {
            private:
                /* Bitmask marking allocated child nodes */
                std::bitset<32> ptrmask;
                /* Sparse list of child nodes corresponding to `ptrmask` */
                std::vector<Node *> children;
                /* Number of keys stored in subtree rooted by this node */
                size_t count;
                /* Calculate index of child node based on `ptrmask` */
                int getChild(const HashType, const int depth) const;
                //const Node * cgetChild(const HashType, const int depth) const;
            public:
                ~SplitNode();
                /* Implementations of Node virtual functions */
                int insert(HashType, Key, const T&, int depth);
                T * read(HashType, Key, int depth);
                const T * cread(HashType, Key, int depth);
                int remove(HashType, Key, int depth, size_t *);
        };

        class LeafNode : public ReliableHAMT::Node {
            private:
                /* Key-value store for data (expected size == 1, list in case
                 * of hash collisions)
                 */
                std::list<std::pair<Key, T>, Alloc > data;
            public:
                ~LeafNode();
                /* Implementations of Node virtual functions */
                int insert(HashType, Key, const T&, int depth);
                const T * cread(HashType, Key, int depth);
                T * read(HashType, Key, int depth);
                int remove(HashType, Key, int depth, size_t *);
        };

        /* Root node of HAMT */
        SplitNode root;
        /* Instance of Hash template type for hashing keys */
        Hash hasher;

    public:
        int insert(Key, const T&);
        T * read(Key);
        const T * cread(Key);
        int remove(Key);
};

/**** Leaf Node Implementation ****/

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::
insert(HashType hash, Key key, const T& t, int depth)
{
    /* Perform a linear search through collision list for a matching key.
    * If found, simply update the value and return, otherwise add the new
    * key-value pair.
    */
    (void)hash;
    (void)depth;
    for (auto & it : data) {
        //if (Pred(it.first, key)) {
        if (it.first == key) {
            it.second = t;
            return 0;
        }
    }
    data.push_back(std::make_pair(key, t));
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
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::
read(HashType hash, Key key, int depth)
{
    /* Read a key-value pair from the data list, returning a pointer to the
     * value or a null pointer if no matching key was found.
     */
    (void)hash;
    (void)depth;
    for (auto & it : data) {
        //if (Pred(it.first, key)) {
        if (it.first == key) {
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
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::
cread(HashType hash, Key key, int depth)
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
    /* Search for matching key-value pair, removing it if found. Update the
     * parent's `childcount` with the size of the data list.  Return 1 if
     * a value was successfully removed, otherwise 0.
     */
    (void)hash;
    (void)depth;
    int rv = 0;
    for (auto & it : data) {
        //if (Pred(it.first, key)) {
        if (it.first == key) {
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
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::LeafNode::
~LeafNode()
{
}


/**** Split Node Implementation ****/

template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
getChild(const HashType hash, const int depth) const
{
    /* Get the subhash for this level and check if the appropriate child bit
     * is set. If not, the child is not present, so return with error.
     */
    HashType shash = ReliableHAMT::subhash(hash, depth);
    if (0 == ptrmask.test(shash)) { // Child is not present
        return -1;
    }

    /* If the child is present, calculate it's index by counting the number of
     * less significant bits set (AND with 2^shash - 1).
     */
    std::bitset<32> mask((1 << shash) - 1);
    size_t chldidx = (ptrmask & mask).count(); // inside [0, 31]
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
    /* Avoid typing long gross template type multiple times */
    using RHAMT = ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>;

    /* Find the appropriate index into the children array. If the child does
     * not already exist, allocate a new node (leaf or split, based on depth)
     * and set bitmask. The index to insert is recalculated to maintain proper
     * ordering.
     */
    int chldidx = getChild(hash, depth);
    if (-1 == chldidx) {
        HashType shash = ReliableHAMT::subhash(hash, depth);
        std::bitset<32> andmask((1 << shash) - 1);
        chldidx = (ptrmask & andmask).count(); // inside [0, 31]
        RHAMT::Node * newnode;
        if (depth == (maxdepth-1))
            { newnode = new RHAMT::LeafNode(); }
        else
            { newnode = new RHAMT::SplitNode(); }
        children.insert(children.begin() + chldidx, newnode);
        ptrmask.set(chldidx);
    }

    /* Assuming child exists (or was created), recursively insert new value and
     * update the child count appropriately.
     */
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
read(HashType hash, Key key, int depth)
{
    /* Recursively read from the appropriate child, returning NULL if the
     * child doesn't exist.
     */
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
cread(HashType hash, Key key, int depth)
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
    /* Attempt to remove value. If child is not allocated, simply return 0 to
     * indicate no removal.
     */
    int chldidx = getChild(hash, depth);
    if (chldidx == -1)
        { return 0; }

    /* Recursively call remove on the child node, updating count to reflect
     * updated number of keys stored
     */
    int rv = children[chldidx]->remove(hash, key, depth+1, childcount);
    count -= rv;

    /* check if childcount is zero and deallocate child if necessary */
    if (0 == *childcount) {
        delete children[chldidx];
        children.erase(children.begin() + chldidx);
    }

    /* Update childcount with current node's key count */
    *childcount = count;
    return rv;
}


template<   class Key,
            class T,
            class HashType  = uint32_t,
            class Hash  = std::hash<Key>,
            class Pred  = std::equal_to<Key>,
            class Alloc = std::allocator< std::pair<const Key, T> >
        >
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::SplitNode::
~SplitNode()
{
    for (Node * child : children) {
        if (child) {
            delete child;
        }
    }
}


template<   class Key,
            class T,
            class HashType,
            class Hash,
            class Pred,
            class Alloc
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
insert(Key key, const T& t)
{
    HashType hash = hasher(key);
    return root.insert(hash, key, t, 0);
}


template<   class Key,
            class T,
            class HashType,
            class Hash,
            class Pred,
            class Alloc
        >
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
read(Key key)
{
    HashType hash = hasher(key);
    return root.read(hash, key, 0);
}


template<   class Key,
            class T,
            class HashType,
            class Hash,
            class Pred,
            class Alloc
        >
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
cread(Key key)
{
    HashType hash = hasher(key);
    return root.cread(hash, key, 0);
}


template<   class Key,
            class T,
            class HashType,
            class Hash,
            class Pred,
            class Alloc
        >
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
remove(Key key)
{
    HashType hash = hasher(key);
    return root.remove(hash, key, 0);
}

#endif // RHAMT_HPP
