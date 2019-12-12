## Guidelines

1. For `std::allocator` only use `allocate` and `deallocate` member functions
as all other member functions are deprecated as of C++20.

## Error Model

Faulty-RAM error model

### Safety

We consider instruction memory, as well as "voting" memory to be hardware
fault-tolerant.
