# MemGuard

MemGuard is a custom memory allocator written from scratch in C, designed to replace standard library memory allocation functions (`malloc`, `calloc`, `realloc`, and `free`). 

It includes robust memory protection and tracking features:
- **Heap Corruption Detection**: Uses magic numbers (canaries) placed before and after the user data to detect buffer overflows and double-frees.
- **Statistics Tracking**: Exposes real-time stats (total allocated, active allocations, largest free block, and corruption events).
- **Block Splitting & Coalescing**: Reduces internal and external memory fragmentation.
- **Large & Small Allocations**: Uses `sbrk` for small allocations and `mmap` for allocations >= 128KB.

## Building & Usage

### Prerequisites
- GCC compiler
- Make
- POSIX-compliant system (Linux, macOS, or Windows Subsystem for Linux)

### Building
```bash
make           # build the static library
make tests     # run all test suites (requires POSIX environment)
```

To use MemGuard in your own C projects, include `allocator.h` and link against the compiled `liballocator.a` static library.
