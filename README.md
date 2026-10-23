# MemGuard

**MemGuard** is a custom memory allocator built from scratch in C. It replaces the standard `malloc`, `calloc`, `realloc`, and `free` functions with a secure, instrumented implementation that can detect heap corruption, track allocation statistics, and manage memory efficiently using both `sbrk` and `mmap` system calls.

---

## What Problem Does This Solve?

In standard C, when your program writes more data into an allocated buffer than it was given (a *buffer overflow*), or when it tries to free the same memory twice (a *double-free*), the result is silent, undefined behavior — your program might crash randomly, produce wrong results, or worse, be exploitable.

**MemGuard solves this** by placing secret "canary" values around every allocation. If anything corrupts those values, MemGuard catches it immediately and reports the exact type of corruption — instead of silently breaking.

---

## Features

| Feature | Description |
|---|---|
| 🛡️ **Buffer Overflow Detection** | A `0xDEADBEEF` canary is placed after every allocation. If your code writes past the boundary, MemGuard catches it on `free()`. |
| ☠️ **Double-Free Detection** | Every block tracks whether it's already been freed. A second `free()` on the same pointer is caught and reported instead of corrupting the heap. |
| 📊 **Allocation Statistics** | Call `get_allocator_stats()` at any point to get total bytes allocated, number of active allocations, largest free block, and a count of corruption events caught. |
| ✂️ **Block Splitting** | When a free block is much larger than needed, MemGuard splits it to avoid wasting memory (*internal fragmentation*). |
| 🔗 **Block Coalescing** | When adjacent blocks are both free, MemGuard merges them into a single larger block to prevent *external fragmentation*. |
| 🗺️ **Dual Allocation Strategy** | Small allocations (< 128 KB) are served from a managed heap via `sbrk`. Large allocations (≥ 128 KB) go directly through `mmap` for efficiency. |
| 🪟 **Windows Compatible** | A thin compatibility shim (`windows_compat.h`) translates POSIX system calls into native Windows `VirtualAlloc` calls, so MemGuard compiles and runs on Windows too. |

---

## How It Works

Every allocation has a hidden **metadata header** prepended to it that MemGuard uses internally:

```
[ Block Header ] [ Front Canary ] [ ---- User Data ---- ] [ Rear Canary ]
```

- **Block Header** — stores size, free/used status, whether it came from `mmap`, and a pointer to the next block.
- **Front & Rear Canaries** — both set to `0xDEADBEEF`. On every `free()` and `realloc()`, MemGuard verifies these values haven't changed. If they have, corruption is reported.

All `sbrk`-based blocks are linked together in a **free list**. When you call `malloc(n)`, MemGuard walks the list to find a suitable free block. If none is found, it requests more memory from the OS.

---

## API

```c
// Standard allocation functions (drop-in replacements)
void *malloc(size_t size);
void *calloc(size_t n, size_t size);   // zero-initializes; guards against overflow
void *realloc(void *ptr, size_t size);
void  free(void *ptr);                 // detects double-free and buffer overflow

// MemGuard-specific
allocator_stats get_allocator_stats(void);
```

### `allocator_stats` struct

```c
typedef struct {
    size_t total_allocated;            // currently active bytes
    size_t active_allocations;         // number of live allocations
    size_t largest_free_block;         // size of largest available free block
    size_t corruption_events;          // total canary/double-free violations caught
    size_t total_cumulative_allocated; // all-time bytes allocated (never decrements)
} allocator_stats;
```

---

## Building

**Prerequisites:** GCC, `make`, and a POSIX system (Linux/macOS) or Windows with MinGW.

```bash
# Build all targets
make

# Run the full test suite (basic, edge cases, canary, calloc, stats)
make tests

# Run the benchmark
make bench

# Build as a static library (liballocator.a)
make lib
```

To use MemGuard in your own project:

```bash
# 1. Build the library
make lib

# 2. Compile your code against it
gcc -I./include my_program.c -L./build -lallocator -o my_program
```

---

## Tests

| Test File | What It Covers |
|---|---|
| `tests/test_basic.c` | Core malloc/free/realloc/calloc workflows |
| `tests/test_edge_cases.c` | Large allocations, fragmentation, realloc edge cases |
| `tests/test_canary.c` | Normal cycle (no false positives), deliberate overflow, deliberate double-free |
| `tests/test_calloc.c` | Zero-init, `calloc(0, n)`, `calloc(n, 0)`, integer overflow input |
| `tests/test_stats.c` | Active allocation counts, corruption counter increments |

---

## Project Structure

```
MemGuard/
├── include/
│   ├── allocator.h          # Public API and allocator_stats struct
│   └── windows_compat.h     # Windows compatibility layer (VirtualAlloc shim)
├── src/
│   └── allocator.c          # Core allocator implementation
├── tests/
│   ├── test_basic.c
│   ├── test_edge_cases.c
│   ├── test_canary.c
│   ├── test_calloc.c
│   ├── test_stats.c
│   └── benchmark.c
├── examples/
│   └── my_program.c
├── progress/
│   └── status.md
├── MemGuard_Documentation.md  # Full architecture & design documentation
└── Makefile
```

---

## Notes

- **Not thread-safe.** Do not use from multiple threads without adding mutex locks.
- `sbrk` is deprecated on macOS but still functional.
- No compaction or defragmentation — freed blocks are coalesced with neighbors but not moved.

---

## License

MIT
