#ifndef _RHAMT_HPP
#define _RHAMT_HPP
#include "voter.hpp"
#include <array>
#include <vector>
#include <list>
#include <bitset>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <csetjmp>
#include <csignal>
#include <cassert>
#include <iostream>

template<class Key, class T, unsigned FT = 0, class HashType = uint32_t,
         class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<std::pair<const Key, T>>>
class Injector;

std::jmp_buf env;
[[noreturn]] void sigsegv_handler(int signal) {
    if (SIGSEGV == signal) {
        longjmp(env, 1);
    }
    else {
        exit(1);
    }
}

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

    const mapped_type * insert(const key_type&, const mapped_type&);
    // int insert(const value_type&);

    int remove(const key_type&);
    // void clear();

    // mapped_type *       read(const key_type&);
    const mapped_type * read(const key_type&);


protected:
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

    /* Virtual base class */
    class Node {
    public:
        enum class optype { read, remove, insert };
        using omtr = std::optional<std::reference_wrapper<const mapped_type>>;
        virtual ~Node() {};
        virtual const mapped_type * fast_traverse(
                const hash_type&, const key_type&, omtr, const optype,
                const int depth, Node * root, size_t * child_count) = 0;
        virtual const mapped_type * safe_traverse(
                const hash_type&, const key_type&, omtr, const optype,
                const int depth, Node * root, size_t * child_count) = 0;
    };

    class SplitNode : public ReliableHAMT::Node {
    public:
        /* Avoid typing long gross template type multiple times */
        using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
        using optype = typename RHAMT::Node::optype;
        using omtr = std::optional<std::reference_wrapper<const mapped_type>>;

        /* Redundant arrays of child pointers */
        std::array<Node *, ft> children [nchldrn];

        /* Number of keys stored in subtree rooted by this node */
        size_t _count;
        /* Calculate index of child node based on `ptrmask` */
        int getChild(const hash_type&, const int depth);
        /* Voting object for comparing redundant data */
        static constexpr Voter<std::array<Node *, ft>, FT> childvoter =
                                        Voter<std::array<Node *, ft>, FT>();


    public:
        SplitNode() : _count(0) {
            for (int i = 0; i < nchldrn; ++i)
                for (int j = 0; j < ft; ++j)
                    children[i][j] = nullptr;
        }
        ~SplitNode();
        const mapped_type * fast_traverse(
            const hash_type&, const key_type&, omtr, const optype,
            const int depth, Node * root, size_t * child_count);
        const mapped_type * safe_traverse(
            const hash_type&, const key_type&, omtr, const optype,
            const int depth, Node * root, size_t * child_count);

        size_t getCount() const { return _count; };
    };

    class LeafNode : public ReliableHAMT::Node {
    public:
        /* Avoid typing long gross template type multiple times */
        using RHAMT = ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>;
        using optype = typename RHAMT::Node::optype;
        using omtr = std::optional<std::reference_wrapper<const mapped_type>>;

        // Key-value store for data (expected size == 1, list in case
        // of hash collisions)
        std::list<value_type, allocator_type> data;
        key_equal key_eq;
        std::array<hash_type, ft> hashes;
        /* Voting object for comparing redundant data */
        static constexpr Voter<std::array<hash_type, ft>, FT> hashvoter =
                                     Voter<std::array<hash_type, ft>, FT>();
        const mapped_type * insert(const key_type&, omtr);
        int remove(const key_type&, size_t *);
        const mapped_type * read(const key_type&);
        const mapped_type * apply_op(const key_type &, omtr,
                                     size_t * ccount, const optype);
    public:
        LeafNode(const hash_type& h) {
            for (int i = 0; i < ft; ++i)
                hashes[i] = h;
        }
        ~LeafNode();

        const mapped_type * fast_traverse(
            const hash_type&, const key_type&, omtr, const optype,
            const int depth, Node * root, size_t * child_count);
        const mapped_type * safe_traverse(
            const hash_type&, const key_type&, omtr, const optype,
            const int depth, Node * root, size_t * child_count);
    };

    SplitNode _root;
    hasher hasher_function;

    friend class Injector<Key, T, FT, HashType, Hash, Pred, Alloc>;
};

/**** Leaf Node Implementation ****/
template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::safe_traverse(const HashType& hash, const Key& key, omtr val,
                        const optype op, const int depth,
                        Node * root, size_t * ccount)
{
    /* Verify this node is correct by comparing the provided hash with the
     * agreed upon value after voting. If the hash is incorrect, we are not
     * at the correct node, so we must repair the structure.
     */
    (void)depth;
    hashvoter(hashes);
    if (hash != hashes[0]) {
        throw("Uh-oh, an unrepairable error was found in leaf node");
    }
    return apply_op(key, val, ccount, op);
}

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::fast_traverse(const HashType& hash, const Key& key, omtr val,
                        const optype op, const int depth,
                        Node * root, size_t * ccount)
{
    (void)depth;
    if constexpr (FT == 0) {
        return apply_op(key, val, ccount, op);
    }
    else {
        try {
            hashvoter(hashes);
        }
        catch (const std::runtime_error& e) {
            return root->safe_traverse(hash, key, val, op, 0, root, ccount);
        }
        if (hash != hashes[0]) {
            return root->safe_traverse(hash, key, val, op, 0, root, ccount);
        }
        else {
            return apply_op(key, val, ccount, op);
        }
    }
}

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::apply_op(const Key &key, omtr val,
                   size_t * ccount, const optype op)
{
    const T * retval = nullptr;
    switch (op) {
        case RHAMT::Node::optype::insert:
            retval = this->insert(key, val);
            break;
        case RHAMT::Node::optype::remove:
            retval = reinterpret_cast<const T*>(this->remove(key, ccount));
            break;
        case RHAMT::Node::optype::read:
            retval = this->read(key);
            break;
        default:
            throw "invalid operation";
    }
    return retval;
}

template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::insert(const Key& key, omtr val)
{
    /* Normally, we don't expect multiple keys to map to the same hash, since
     * most key types have a strong hash function available. If a collision
     * does occur, we must search through the list to find the matching key.
     */
    if (!val.has_value())
        throw "wtf why does omtr not have value in insert?!";

    const T& tval = val.value().get();

    for (auto & it : data) {
        if (key_eq(it.first, key)) {
            it.second = tval;
            return &it.second;
        }
    }

    /* If no match was found, insert the new key-value pair */
    data.push_back(std::make_pair(key, tval));
    return &data.back().second;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::remove(const Key& key, size_t *childcount)
{
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

    if (childcount)
        *childcount = data.size();
    return rv;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
LeafNode::read(const Key& key)
{
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
LeafNode::~LeafNode() { }


/**** Split Node Implementation ****/


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::safe_traverse(const HashType & hash, const Key & key, omtr val,
                         const optype op, const int depth, Node * root,
                         size_t * ccount)
{
    int child_idx = getChild(hash, depth);
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
    const T * rv = children[child_idx][0]->safe_traverse(
                                    hash, key, val, op, depth+1, root, ccount);

    uintptr_t crv = reinterpret_cast<uintptr_t>(rv);
    switch (op) {
        case RHAMT::Node::optype::insert:
            _count += crv;
            break;
        case RHAMT::Node::optype::remove:
            _count -= crv;
            *ccount = _count;
            break;
        case RHAMT::Node::optype::read:
            break;
        default:
            throw "WTF???";
    }

    return rv;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::fast_traverse(const HashType & hash, const Key & key, omtr val,
                         const optype op, const int depth, Node * root,
                         size_t * ccount)
{
    if constexpr (FT == 0) {
        int child_idx = getChild(hash, depth);
        if (nullptr == children[child_idx][0]) {
            RHAMT::Node * newnode;
            if (depth == (maxdepth-1))
                { newnode = new RHAMT::LeafNode(hash); }
            else
                { newnode = new RHAMT::SplitNode(); }
            children[child_idx][0] = newnode;
        }
        return children[getChild(hash, depth)][0]->fast_traverse(
                hash, key, val, op, depth+1, root, ccount);
    }
    else if constexpr (FT > 0) {
        const T * retval;
        static struct sigaction sa = {sigsegv_handler, (unsigned)NULL, 0,
                                      0, (unsigned)NULL};
        sa.sa_flags = SA_NODEFER | SA_RESETHAND;
        if (depth == 0) {
            // Turn On Signal Handler
            if (0 != sigaction(SIGSEGV, &sa, NULL)) {
                perror("sigaction");
                exit(1);
            }

            if (setjmp(env) > 0) {
                return root->safe_traverse(hash, key, val, op, 0, root, ccount);
            }
        }

        retval = children[getChild(hash, depth)][0]->fast_traverse(
                                        hash, key, val, op, depth+1, root, ccount);
        signal(SIGSEGV, SIG_DFL);
        return retval;
    }
    printf("WTF\n");
    exit(1);
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
inline int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
SplitNode::getChild(const HashType& hash, const int depth)
{
    HashType shash = ReliableHAMT::subhash(hash, depth);
    return shash;
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
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
insert(const Key& key, const T& tval)
{
    size_t hash = hasher_function(key);
    size_t cc;
    const mapped_type * rv;
    auto val = std::optional<std::reference_wrapper<const T>>(
            std::reference_wrapper<const T>(tval));
    rv = _root.fast_traverse(
            hash, key, val, Node::optype::insert, 0, &_root, &cc);
    return rv;
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
int
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
remove(const Key& key)
{
    HashType hash = hasher_function(key);
    size_t cc;
    auto val = std::optional<std::reference_wrapper<const T>>();
    return reinterpret_cast<uintptr_t>(_root.fast_traverse(
                hash, key, val, Node::optype::remove, 0, &_root, &cc));
}


template <class Key, class T, unsigned FT, class HashType, class Hash, class Pred, class Alloc>
const T *
ReliableHAMT<Key, T, FT, HashType, Hash, Pred, Alloc>::
read(const Key& key)
{
    HashType hash = hasher_function(key);
    size_t cc;
    const mapped_type * rv;
    auto val = std::optional<std::reference_wrapper<const T>>();
    rv = _root.fast_traverse(
            hash, key, val, Node::optype::read, 0, &_root, &cc);
    return rv;
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
#endif // RHAMT_HPP
