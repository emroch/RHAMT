## Guidelines

1. For `std::allocator` only use `allocate` and `deallocate` member functions
as all other member functions are deprecated as of C++20.

## Error Model

### Safety

We consider instruction memory, as well as "voting" memory to be hardware
fault-tolerant.  Also, we consider the counts to be fault tolerant because
we don't really want to deal with all that.
