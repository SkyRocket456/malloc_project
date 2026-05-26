#pragma once
// =============================================================================
// my_malloc.h
// =============================================================================
// You do NOT need to edit this file.
//
// It defines:
//   - the Block struct that sits before every allocation on the heap
//   - alignment and size constants
//   - the four functions you must implement in my_malloc.cpp
// =============================================================================

#include <cstddef>    // size_t
#include <cstdint>    // uintptr_t, uint8_t

// ─────────────────────────────────────────────────────────────────────────────
//  Constants  (feel free to adjust these as you design your allocator)
// ─────────────────────────────────────────────────────────────────────────────

// Every pointer returned by my_malloc must be a multiple of ALIGNMENT.
static constexpr size_t ALIGNMENT = 16;

// The smallest block your allocator will ever create (header + usable space).
// You may change this once you decide on your header layout.
static constexpr size_t MIN_BLOCK_SIZE = 48;

// When the heap needs to grow, request at least this many bytes from the OS.
static constexpr size_t MIN_HEAP_CHUNK = 4144;

// ─────────────────────────────────────────────────────────────────────────────
//  Alignment helper
//  Round n up to the next multiple of align (align must be a power of two).
// ─────────────────────────────────────────────────────────────────────────────
inline size_t align_up(size_t n, size_t align) {
    return (n + align - 1) & ~(align - 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Block  — sits immediately before every allocated or free region on the heap
// ─────────────────────────────────────────────────────────────────────────────
//
//  Memory layout of one heap block:
//
//    low address
//    ┌──────────────┐  ← Block*  (the header you manage)
//    │   Block      │
//    │   (header)   │
//    ├──────────────┤  ← payload  (what my_malloc returns to the caller)
//    │              │
//    │  user data   │
//    │              │
//    └──────────────┘
//    high address
//
//  FIELDS TO FILL IN:
//    Add whatever fields your design needs.
//    Common choices:
//      - size of this block (including header)
//      - a flag: is this block currently allocated?
//      - pointers to the previous and next free block (for an explicit free list)
//
//  You must keep sizeof(Block) a multiple of ALIGNMENT so that the payload
//  that follows it is also aligned.
//
struct Block {
       size_t  size;        // size in bytes of memory user asked for (payload)
       Block*  prev;        // previous block in free list
       Block*  next;        // next block in free list
       bool is_free;         // extra field just to hit the next multiple of 16

    // ── helpers you may find useful (implement in my_malloc.cpp if you use them)

    // Returns a pointer to the usable memory that follows this header.
    void* payload() {
        return reinterpret_cast<uint8_t*>(this) + sizeof(Block);
    }

    // Returns the Block header that owns a given payload pointer.
    static Block* from_payload(void* ptr) {
        return reinterpret_cast<Block*>(
            reinterpret_cast<uint8_t*>(ptr) - sizeof(Block));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Public API  — implement these four functions in my_malloc.cpp
// ─────────────────────────────────────────────────────────────────────────────

// Allocate at least `size` bytes; return a 16-byte-aligned pointer.
// Return nullptr if size == 0 or the heap cannot grow.
void* my_malloc(size_t size);

// Release the memory pointed to by `ptr` (previously returned by my_malloc).
// If ptr is nullptr, do nothing.
void  my_free(void* ptr);

// Resize the allocation at `ptr` to `new_size` bytes, preserving contents.
// Behaves like my_malloc when ptr == nullptr.
// Behaves like my_free   when new_size == 0.
void* my_realloc(void* ptr, size_t new_size);

// Allocate nmemb * size bytes, zero-initialised.
// Return nullptr on overflow or allocation failure.
void* my_calloc(size_t nmemb, size_t size);

// ─────────────────────────────────────────────────────────────────────────────
//  Diagnostics  (optional but strongly recommended — implement in my_malloc.cpp)
// ─────────────────────────────────────────────────────────────────────────────

// Print every block on the heap (address, size, free/allocated).
void heap_dump();

// Verify heap invariants; return true if everything looks correct.
// Suggested checks:
//   1. Every block's size is > 0 and a multiple of ALIGNMENT.
//   2. No two adjacent blocks are both free (coalescing invariant).
//   3. Every block in the free list is actually marked free.
bool heap_check();
