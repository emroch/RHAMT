#ifndef _RHAMT_HPP
#define _RHAMT_HPP
#include "voter.hpp"
#include <array>
#include <vector>
#include <list>
#include <bitset>
#include <cstdint>
#include <functional>


template <class Key, class T, unsigned FT = 0, class HashType = uint32_t,
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

    // mapped_type *       read(const key_type&);
    const mapped_type * read(const key_type&);


private:
    /* Number of children for each node */
    static constexpr int nchldrn = 32;
    /* Size of index for child node array */
    static constexpr int nlog2chldrn = 5;
    /* Max depth of tree, based on number of children at each level */
    static constexpr int soh = sizeof(HashType) * 8;
    static constexpr int maxdepth = (soh + (nlog2chldrn - 1)) / nlog2chldrn;

    static constexpr int ft = 2 * FT + 1;
    static_assert(FT < 8,
            "Fault tolerance values greater than 7 are not supported");

    /* Extract subhash for indexing child array at depth */
    static constexpr hash_type subhash(const hash_type hash, const int depth)
    {
        // Shift the full hash by `nlog2chldrn` for each level of the tree,
        // then mask the lowest `nlog2chldrn` bits to get the subhash
        return (hash >> (nlog2chldrn * depth)) & ((1 << nlog2chldrn) - 1);
    }

    void traverse_fast(const hash_type&);
    void traverse_safe(const hash_type&);

    /* If an error is detected, repair the trie along that path */
    void repair(const hash_type&);

    /* Virtual base class */
    class Node {
    public:
        virtual ~Node() {};
        // Returns 1 if a new key is inserted or 0 if key is updated
        virtual int insert(const hash_type&, const key_type&,
                           const mapped_type&, int depth) = 0;
        // Remove a value from HAMT with given hash and key, returning
        // 1 if a leaf is destroyed, otherwise 0
        virtual int remove(const hash_type&, const key_type&,
                           int depth, size_t *) = 0;
        // Retrieve a value from HAMT with given hash and key
        virtual const mapped_type * read(const hash_type&, const key_type&,
                                   int depth) = 0;
    };

    class SplitNode : public ReliableHAMT::Node {
    private:
        /* Avoid typing long gross template type multiple times */
        using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;

        /* Redundant arrays of child pointers */
        std::array<Node *, ft> children [nchldrn];

        /* Number of keys stored in subtree rooted by this node */
        size_t _count;
        /* Calculate index of child node based on `ptrmask` */
        int getChild(const hash_type&, const int depth);
        /* Voting object for comparing redundant data */
        Voter<std::array<Node *, ft>, FT> childvoter =
                                        Voter<std::array<Node *, ft>, FT>();
    public:
        SplitNode() : _count(0) {
            for (int i = 0; i < nchldrn; ++i)
                for (int j = 0; j < ft; ++j)
                    children[i][j] = nullptr;
        }
        ~SplitNode();

        int insert(const hash_type&, const key_type&,
                   const mapped_type&, int depth);
        int remove(const hash_type&, const key_type&, int depth, size_t *);
        const mapped_type * read(const hash_type&, const key_type&, int depth);
        size_t getCount() const { return _count; };
    };

    class LeafNode : public ReliableHAMT::Node {
    private:
        /* Avoid typing long gross template type multiple times */
        using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;

        // Key-value store for data (expected size == 1, list in case
        // of hash collisions)
        std::list<value_type, allocator_type> data;
        key_equal key_eq;
        std::array<hash_type, ft> hashes;
        /* Voting object for comparing redundant data */
        Voter<std::array<hash_type, ft>, FT> hashvoter =
                                     Voter<std::array<hash_type, ft>, FT>();
    public:
        LeafNode(const hash_type& h) {
            for (int i = 0; i < ft; ++i)
                hashes[i] = h;
        }
        ~LeafNode();

        int insert(const hash_type&, const key_type&,
                   const mapped_type&, int depth);
        int remove(const hash_type&, const key_type&, int depth, size_t *);
        const mapped_type * read(const hash_type&, const key_type&, int depth);
    };

    SplitNode _root;
    hasher hasher_function;
};

/**** Leaf Node Implementation ****/

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::insert(const HashType& hash, const Key& key, const T& val, int depth)
{
    (void)depth;

    /* Verify this node is correct by comparing the provided hash with the
     * agreed upon value after voting. If the hash is incorrect, we are not
     * at the correct node, so we must repair the structure.
     */
    hashvoter(hashes);
    if (hash != hashes[0]) {
        printf("Uh-oh, an error was found, entering repair mode");
    }

    /* Normally, we don't expect multiple keys to map to the same hash, since
     * most key types have a strong hash function available. If a collision
     * does occur, we must search through the list to find the matching key.
     */
    for (auto & it : data) {
        if (key_eq(it.first, key)) {
            it.second = val;
            return 0;
        }
    }

    /* If no match was found, insert the new key-value pair */
    data.push_back(std::make_pair(key, val));
    return 1;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::remove(const HashType& hash, const Key& key, int depth,
                 size_t *childcount)
{
    (void)depth;

    /* Verify this node is correct by comparing the provided hash with the
     * agreed upon value after voting. If the hash is incorrect, we are not
     * at the correct node, so we must repair the structure.
     */
    hashvoter(hashes);
    if (hash != hashes[0]) {
        printf("Uh-oh, an error was found, entering repair mode");
        //RHAMT::repair(hash);
    }
   
    /* Search for matching key-value pair, removing it if found. Update the
     * parent's `childcount` with the size of the data list.  Return 1 if
     * a value was successfully removed, otherwise 0.
     */
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


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::read(const HashType& hash, const Key& key, int depth)
{
    (void)hash;
    (void)depth;

    /* Verify this node is correct by comparing the provided hash with the
     * agreed upon value after voting. If the hash is incorrect, we are not
     * at the correct node, so we must repair the structure.
     */
    hashvoter(hashes);
    if (hash != hashes[0]) {
        printf("Uh-oh, an error was found, entering repair mode");
        //RHAMT::repair(hash);
    }

    /* Read a key-value pair from the data list, returning a pointer to the
     * value or a null pointer if no matching key was found.
     */
    for (auto & it : data) {
        if (key_eq(it.first, key)) {
            return &it.second;
        }
    }
    return nullptr;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::~LeafNode()
{
}


/**** Split Node Implementation ****/

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
inline int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::getChild(const HashType& hash, const int depth)
{
    HashType shash = ReliableHAMT::subhash(hash, depth);
    return shash;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::insert(const HashType& hash, const Key& key, const T& val, int depth)
{
    int child_idx = getChild(hash, depth);

    if (nullptr == children[child_idx][0]) {
        // vote and check again before modifying the structure to ensure
        // the child pointer really is null.
        childvoter(children[child_idx]);
        if (nullptr == children[child_idx][0]) {
            RHAMT::Node * newnode;
            if (depth == (maxdepth-1))
                { newnode = new RHAMT::LeafNode(hash); }
            else
                { newnode = new RHAMT::SplitNode(); }
            for (int j = 0; j < ft; ++j)
                children[child_idx][j] = newnode;
        }
    }

    // Assume the first pointer is correct (or was just created) and insert
    // new value recursively. Total correctness will be checked at the leaf.
    int rv = children[child_idx][0]->insert(hash, key, val, depth+1);
    _count += rv;
    return rv;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::remove(const HashType& hash, const Key& key, int depth,
                  size_t *childcount)
{
    /* Attempt to remove value. If child is not allocated,
     * simply return 0 to indicate no removal.
     */
    int child_idx = getChild(hash, depth);

    // If child is not allocated, double check (lest we lose track of data)
    // and return zero if there is really no child
    if (nullptr == children[child_idx][0]) {
        childvoter(children[child_idx]);
        if (nullptr == children[child_idx][0])
            return 0;
    }

    /* Recursively call remove on the child node, updating count to reflect
     * updated number of keys stored. Error checking happens at the deepest
     * node without a child.
     */
    int rv = children[child_idx][0]->remove(hash, key, depth+1, childcount);
    _count -= rv;

    /* check if childcount is zero and deallocate child if necessary */
    if (0 == *childcount) {
        // If we made it here, then the recursion reached a valid leaf node,
        // or else the trie was repaired. Thus, children[child_idx][0] must
        // be correct, or the recursion would have failed.  We can ignore the
        // rest of the duplicates.
        delete children[child_idx][0];
        for (int j = 0; j < ft; ++j)
            children[child_idx][j] = nullptr;
    }

    /* Update childcount with current node's key count */
    *childcount = _count;
    return rv;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::read(const HashType& hash, const Key& key, int depth)
{
    /* Recursively read from the appropriate child, returning NULL if the
     * child doesn't exist.
     */
    int child_idx = getChild(hash, depth);

    if (nullptr == children[child_idx][0]) {
        childvoter(children[child_idx]);  // verify pointer is really null
        if (nullptr == children[child_idx][0])
            return nullptr;
    }
    // if pointer is not null, error checking happens at the deepest level
    return children[child_idx][0]->read(hash, key, depth+1);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::~SplitNode()
{
    for (auto &child : children) {
        //childvoter(child);    // TODO: this causes a massive slowdown
        delete child[0];
    }
}


/**** ReliableHAMT Implementation ****/

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
insert(const Key& key, const T& val)
{
    size_t hash = hasher_function(key);
    return _root.insert(hash, key, val, 0);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
remove(const Key& key)
{
    HashType hash = hasher_function(key);
    size_t childcount;
    return _root.remove(hash, key, 0, &childcount);
}


//template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
//T *
//ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
//read(const Key& key)
//{
//    HashType hash = hasher_function(key);
//    return _root.read(hash, key, 0);
//}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
read(const Key& key)
{
    HashType hash = hasher_function(key);
    return _root.read(hash, key, 0);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
bool
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
empty() const
{
    return (0 == _root.getCount());
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
size_t
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
size() const
{
    return _root.getCount();
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
traverse_fast(const HashType& path)
{
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
traverse_safe(const HashType& path)i
{
    using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
    RHAMT::Node* curr_node = &_root;

    
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
void
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
repair(const HashType& path)
{
    using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
    RHAMT::Node* curr_node = &_root;

    
}


#endif // RHAMT_HPP
