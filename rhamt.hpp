#ifndef _RHAMT_HPP
#define _RHAMT_HPP
#include <vector>
#include <list>
#include <bitset>
#include <cstdint>
#include <functional>


template <class Key, class T, class HashType  = uint32_t,
          class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, T>>>
class ReliableHAMT {
public:
    // types
    typedef Key                                         key_type;
    typedef T                                           mapped_type;
    typedef HashType                                    hash_type;
    typedef Hash                                        hasher;
    typedef Pred                                        key_equal;
    typedef Alloc                                       allocator_type;
    typedef std::pair<const key_type, mapped_type>      value_type;
    typedef value_type&                                 reference;
    typedef const value_type&                           const_reference;

    ReliableHAMT() {};
    ~ReliableHAMT() {};

    // TODO: iterators?
    /* typedef unspecified iterator, const_iterator;
    * iterator       begin();
    * iterator       end();
    * const_iterator begin() const;
    * const_iterator end() const;
    */

    bool   empty() const;
    size_t size() const;

    int insert(const key_type&, const mapped_type&);
    // int insert(const value_type&);

    int remove(const key_type&);
    // void clear();

    mapped_type *       read(const key_type&);
    const mapped_type * read(const key_type&) const;


private:
    /* Number of children for each node */
    static constexpr int nchldrn = 32;
    /* Size of index for child node array */
    static constexpr int nlog2chldrn = 5;
    /* Max depth of tree, based on number of children at each level */
    static constexpr int soh = sizeof(HashType) * 8;
    static constexpr int maxdepth = (soh + (nlog2chldrn - 1)) / nlog2chldrn;

    /* Extract subhash for indexing child array at depth */
    static constexpr hash_type subhash(const hash_type hash, const int depth)
    {
        // Shift the full hash by `nlog2chldrn` for each level of the tree,
        // then mask the lowest `nlog2chldrn` bits to get the subhash
        return (hash >> (nlog2chldrn * depth)) & ((1 << nlog2chldrn) - 1);
    }

    /* Virtual base class */
    class Node {
    public:
        virtual ~Node() {};
        /* Returns 1 if a new key is inserted or 0 if key is updated */
        virtual int insert(const hash_type&, const key_type&,
                           const mapped_type&, int depth) = 0;
        /* Remove a value from HAMT with given hash and key, returning
        * 1 if a leaf is destroyed, otherwise 0
        */
        virtual int remove(const hash_type&, const key_type&,
                           int depth, size_t *) = 0;
        /* Retrieve a value from HAMT with given hash and key */
        virtual mapped_type * read(const hash_type&, const key_type&,
                                   int depth) = 0;
        virtual const mapped_type * read(const hash_type&,
                                         const key_type&,
                                         int depth) const = 0;
    };

    class SplitNode : public ReliableHAMT::Node {
    private:
        /* Bitmask marking allocated child nodes */
        std::bitset<nchldrn> ptrmask;
        /* Sparse list of child nodes corresponding to `ptrmask` */
        std::vector<Node *> children;
        /* Number of keys stored in subtree rooted by this node */
        size_t _count;
        /* Calculate index of child node based on `ptrmask` */
        int getChild(const hash_type&, const int depth) const;
        //const Node * getChild(const hash_type&, const int depth) const;
    public:
        SplitNode() : _count(0) {}
        ~SplitNode();

        int insert(const hash_type&, const key_type&,
                   const mapped_type&, int depth);
        int remove(const hash_type&, const key_type&, int depth, size_t *);
        mapped_type * read(const hash_type&, const key_type&, int depth);
        const mapped_type * read(const hash_type&, const key_type&,
                                 int depth) const;
        size_t getCount() const { return _count; };
    };

    class LeafNode : public ReliableHAMT::Node {
    private:
        /* Key-value store for data (expected size == 1, list in case
         * of hash collisions)
         */
        std::list<value_type, allocator_type> data;

        key_equal key_eq;
    public:
        LeafNode() {};
        ~LeafNode();

        int insert(const hash_type&, const key_type&,
                   const mapped_type&, int depth);
        int remove(const hash_type&, const key_type&, int depth, size_t *);
        mapped_type * read(const hash_type&, const key_type&, int depth);
        const mapped_type * read(const hash_type&, const key_type&,
                                 int depth) const;
    };


    SplitNode _root;
    hasher hasher_function;
};

/**** Leaf Node Implementation ****/

template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
LeafNode::insert(const HashType& hash, const Key& key, const T& val, int depth)
{
    /* Perform a linear search through collision list for a matching key.
    * If found, simply update the value and return, otherwise add the new
    * key-value pair.
    */
    (void)hash;
    (void)depth;

    for (auto & it : data) {
        if (key_eq(it.first, key)) {
            it.second = val;
            return 0;
        }
    }
    data.push_back(std::make_pair(key, val));
    return 1;
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
LeafNode::remove(const HashType& hash, const Key& key, int depth,
    size_t *childcount)
    {
        /* Search for matching key-value pair, removing it if found. Update the
        * parent's `childcount` with the size of the data list.  Return 1 if
        * a value was successfully removed, otherwise 0.
        */
        (void)hash;
        (void)depth;

        int rv = 0;
        for (auto & it : data) {
            if (key_eq(it.first, key)) {
                data.remove(it);
                rv = 1;
                break;
            }
        }
        *childcount = data.size();
        return rv;
    }


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
LeafNode::read(const HashType& hash, const Key& key, int depth)
{
    /* Read a key-value pair from the data list, returning a pointer to the
     * value or a null pointer if no matching key was found.
     */
    (void)hash;
    (void)depth;

    for (auto & it : data) {
        if (key_eq(it.first, key)) {
            return &it.second;
        }
    }
    return nullptr;
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
LeafNode::read(const HashType& hash, const Key& key, int depth) const
{
    return this->read(hash, key, depth);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
LeafNode::~LeafNode()
{
}


/**** Split Node Implementation ****/

template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::getChild(const HashType& hash, const int depth) const
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
    std::bitset<nchldrn> mask((1 << shash) - 1);
    size_t child_idx = (ptrmask & mask).count(); // in range [0, nchldrn)
    return child_idx;
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::insert(const HashType& hash, const Key& key, const T& val, int depth)
{
    /* Avoid typing long gross template type multiple times */
    using RHAMT = ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>;

    /* Find the appropriate index into the children array. If the child does
     * not already exist, allocate a new node (leaf or split, based on depth)
     * and set bitmask. The index to insert is recalculated to maintain proper
     * ordering.
     */
    int child_idx = getChild(hash, depth);
    if (-1 == child_idx) {
        HashType shash = ReliableHAMT::subhash(hash, depth);
        std::bitset<nchldrn> mask((1 << shash) - 1);
        child_idx = (ptrmask & mask).count(); // in range [0, nchldrn)
        RHAMT::Node * newnode;
        if (depth == (maxdepth-1))
            { newnode = new RHAMT::LeafNode(); }
        else
            { newnode = new RHAMT::SplitNode(); }
        children.insert(children.begin() + child_idx, newnode);
        ptrmask.set(shash);
    }

    /* Assuming child exists (or was created), recursively insert new value and
     * update the child count appropriately.
     */
    int rv = children[child_idx]->insert(hash, key, val, depth+1);
    _count += rv;
    return rv;
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::remove(const HashType& hash, const Key& key, int depth,
    size_t *childcount)
    {
        /* Attempt to remove value. If child is not allocated, simply return 0 to
        * indicate no removal.
        */
        int child_idx = getChild(hash, depth);
        if (child_idx == -1)
        { return 0; }

        /* Recursively call remove on the child node, updating count to reflect
        * updated number of keys stored
        */
        int rv = children[child_idx]->remove(hash, key, depth+1, childcount);
        _count -= rv;

        /* check if childcount is zero and deallocate child if necessary */
        if (0 == *childcount) {
            delete children[child_idx];
            children.erase(children.begin() + child_idx);
            HashType shash = ReliableHAMT::subhash(hash, depth);
            ptrmask.reset(shash);
        }

        /* Update childcount with current node's key count */
        *childcount = _count;
        return rv;
    }


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::read(const HashType& hash, const Key& key, int depth)
{
    /* Recursively read from the appropriate child, returning NULL if the
     * child doesn't exist.
     */
    int child_idx = getChild(hash, depth);
    if (-1 == child_idx)
        { return nullptr; }
    return children[child_idx]->read(hash, key, depth+1);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::read(const HashType& hash, const Key& key, int depth) const
{
    int child_idx = getChild(hash, depth);
    if (-1 == child_idx)
        { return nullptr; }
    return children[child_idx]->read(hash, key, depth+1);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
SplitNode::~SplitNode()
{
    for (Node * child : children) {
        if (child) {
            delete child;
        }
    }
}


/**** ReliableHAMT Implementation ****/

template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
insert(const Key& key, const T& val)
{
    size_t hash = hasher_function(key);
    return _root.insert(hash, key, val, 0);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
remove(const Key& key)
{
    HashType hash = hasher_function(key);
    size_t childcount;
    return _root.remove(hash, key, 0, &childcount);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
read(const Key& key)
{
    HashType hash = hasher_function(key);
    return _root.read(hash, key, 0);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
read(const Key& key) const
{
    HashType hash = hasher_function(key);
    return _root.read(hash, key, 0);
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
bool
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
empty() const
{
    return (0 == _root.getCount());
}


template <class Key, class T, class HashType, class Hash, class Pred, class Alloc>
size_t
ReliableHAMT<Key, T, HashType, Hash, Pred, Alloc>::
size() const
{
    return _root.getCount();
}

#endif // RHAMT_HPP
