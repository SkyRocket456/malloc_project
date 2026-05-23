// =============================================================================
// my_malloc.cpp
// =============================================================================
// YOUR TASK: implement every function marked TODO below.
//
// Rules:
//   - Do NOT call malloc(), calloc(), realloc(), free(), new, or delete.
//   - Use sbrk() (declared in <unistd.h>) to request memory from the OS.
//   - Every pointer returned by my_malloc must be 16-byte aligned.
//   - Passing all 15 tests in tests.cpp is the goal.
//
// Suggested reading order:
//   1. my_malloc.h  — understand the Block struct and the API you must satisfy
//   2. This file     — read the hints in each TODO section
//   3. tests.cpp    — understand exactly what each test checks
// =============================================================================

#include "my_malloc.h"

#include <cstdio>
#include <unistd.h>     // sbrk()
#include <cstring>      // memset(), memcpy()

// ─────────────────────────────────────────────────────────────────────────────
//  Module-level state
//  Add any global variables your allocator needs here.
//
static Block dummy = {0, nullptr, nullptr, true}; // dummy node
static Block *free_list = &dummy; // head of the free list, head always points to dummy

static void*  heap_start = sbrk(0);  // first byte of the managed heap
static void*  heap_end   = nullptr;  // one-past the last managed byte
// ─────────────────────────────────────────────────────────────────────────────

// TODO: declare your global state here


// =============================================================================
//  INTERNAL HELPERS
//  Declare and implement private helper functions here.
//  You are not required to use any particular set — design what you need.
//
//  Helpers that most implementations will need:
//

void add_block_to_free_list(Block *blk) {
    // New block becomes the first node after the head
    blk->prev = free_list;

    // New block is now in between the head and the 2nd block
    blk->next = free_list->next;

    // if there is actually a 2nd block
    if (free_list->next != nullptr) {
        // the 2nd block's previous is now the new block
        free_list->next->prev = blk;
    }

    // head now points to the new block
    free_list->next = blk;

    blk->is_free = true;
}

void remove_block_from_free_list(Block *blk) {
    // If there is a node behind the block, append the block's next as the previous's next
    if (blk->prev != nullptr) {
        blk->prev->next = blk->next;
    }

    // If there is a node ahead of the block, append the block's previous as the ahead's previous
    if (blk->next != nullptr) {
        blk->next->prev = blk->prev;
    }

    blk->prev = nullptr;
    blk->next = nullptr;
    blk->is_free = false;
}

//  Ask the OS for more memory with sbrk().
//  Format the new region as a free block and return it.
//  Tip: always request at least HEAP_CHUNK bytes so you don't call
//      sbrk() on every small allocation.
//
void extend_heap(size_t size) {
    if (heap_start == nullptr) {
        heap_start = sbrk(0);  // set once on first call
    }

    size_t memory_needed = (size > MIN_HEAP_CHUNK) ? size : MIN_HEAP_CHUNK + sizeof(Block);

    // Ask OS to extend the heap buy 4096 + 32 bytes
    void *raw = sbrk(memory_needed);

    heap_end = static_cast<char *>(raw) + memory_needed;

    // Cast the pointer to that memory as a Block. Now the first 32 bytes can be used for Block struct activities
    // We use static cast so the compiler can at least check if the operation is feasible
    auto *new_block = static_cast<Block *>(raw);

    // Initialize the new block on the free list, wait to initialize next
    new_block->size = memory_needed;

    add_block_to_free_list(new_block);
}

// If blk is much larger than needed, carve off the excess and put he remainder back into the free list.
// Only split if the remainder would be >= MIN_BLOCK_SIZE.
// This function splits the block and appends the new block from the split to the front of the list


Block *split_block(Block *current_block, const size_t memory_needed) {
    // Calculate the offset required for the split. This will be where the new block pointer starts
    long split_size_block = static_cast<long>(current_block->size) - static_cast<long>(memory_needed);

    // If there isn't to split off, return the original block
    if (split_size_block < static_cast<long>(MIN_BLOCK_SIZE)) {
        return current_block;
    }

    // New block is created from the split. Pointer points to where the offset value is
    auto *new_block = reinterpret_cast<Block *>(reinterpret_cast<char *>(current_block) + split_size_block);

    // Initialize new block
    new_block->size = memory_needed;

    add_block_to_free_list(new_block);

    // Change size of block that was split
    current_block->size = split_size_block;

    return new_block;
}

// Walk your free list and return the first (or best) block that is at least `needed` bytes in total size.
// Return nullptr if no such block exists.

Block *find_suitable_block(const size_t memory_needed) {
    // Summon the head of the free list
    Block *blk = free_list;

    // Keep looping until you find a block big enough
    while (blk != nullptr) {
        if (blk->size >= memory_needed) {
            return blk;
        }
        blk = blk->next;
    }
    return nullptr;
}


//
// void coalesce(Block* blk) {
//     // If there is a block ahead to merge with
//     if (blk->next != nullptr) {
//         // Create new total
//         size_t new_total = blk->prev.blk->size + blk->next->size;
//         blk->size = new_total;
//
//
//     }
// }
//      Merge blk with adjacent free neighbours to reduce fragmentation.
//      Boundary-tag coalescing (storing the block size at the *end* of
//      each block) lets you find the previous block in O(1); this is
//      optional but earns the "coalescing" test.
//
// =============================================================================

// TODO: implement your helper functions here


// =============================================================================
//  my_malloc
// =============================================================================
//
//  Steps (one common approach):
//    1. Return nullptr immediately if size == 0.
//    2. Compute the total block size you need:
//         total = align_up(size + sizeof(Block), ALIGNMENT)
//       Make sure total >= MIN_BLOCK_SIZE.
//    3. Search your free list for a suitable block (find_block).
//    4. If none found, call extend_heap to grow the heap AKA ask the OS for more memory
//    5. Optionally split the block if it is much larger than needed.
//    6. Mark the block as allocated.
//    7. Return blk->payload().
//
void *my_malloc(size_t size) {
    // If the size of memory requested is more than 0, continue
    if (size != 0) {

        // True sized needed for the block (header + payload). Round up to a multiple of 16
        const size_t memory_needed = align_up(size, 16) + sizeof(Block);

        // Find a good free block in the free list
        Block *free_block = find_suitable_block(memory_needed);

        // If there is no block returned
        if (free_block == nullptr) {
            // If there is no free blocks, extend the heap
            extend_heap(memory_needed);

            // The free block is now the new memory we gained from the heap
            free_block = free_list->next;
        }

        // If there is enough space left in the current block to be a block on its own after a split, we can split it
        free_block = split_block(free_block, memory_needed);

        remove_block_from_free_list(free_block);

        return free_block->payload();
    }

    // heap_dump();   // For testing only
    return nullptr;
}

// =============================================================================
//  my_free
// =============================================================================
//
//  Steps:
//    1. If ptr is nullptr, return immediately.
//    2. Recover the Block header:  Block* blk = Block::from_payload(ptr);
//    3. Mark the block as free.
//    4. Add it back to your free list.
//    5. (Optional but recommended) Coalesce with adjacent free blocks.
//
void my_free(void *ptr) {
    if (ptr != nullptr) {
        Block *blk = Block::from_payload(ptr);
        add_block_to_free_list(blk);
    }

    // heap_dump();   // For testing only
}

// =============================================================================
//  my_realloc
// =============================================================================
//
//  Steps:
//    1. If ptr == nullptr, behave exactly like my_malloc(new_size).
//    2. If new_size == 0, behave exactly like my_free(ptr); return nullptr.
//    3. Otherwise:
//         a. Find out how many payload bytes the current block holds.
//         b. If new_size fits in the current block, you can return ptr as-is
//            (or split the block to avoid wasting space — optional).
//         c. Otherwise, my_malloc(new_size), memcpy the old data, my_free(ptr).
//
void *my_realloc(void *ptr, size_t new_size) {
    // If they want a new memory allocation with new size but dont send a pointer, it is basically malloc
    if (ptr == nullptr) {
        return my_malloc(new_size);
    }

    // If they want a new size of 0, it is the same as freeing it
    if (new_size == 0) {
        my_free(ptr);
        return nullptr;
    }

    Block *blk = Block::from_payload(ptr);

    // Case 1: If new size is equal to the current block already
    if (blk->size == new_size) {
        return ptr;
    }

    Block* free_block = split_block(blk, new_size);
    void* p;
    if (free_block == blk) {
        p = my_malloc(new_size);
    }
    else {
        p = free_block->payload();
    }
    p = memcpy(p, ptr, new_size);
    return p;
}

// =============================================================================
//  my_calloc
// =============================================================================
//
//  Steps:
//    1. Check for overflow: if nmemb != 0 && total/nmemb != size → return nullptr.
//    2. Call my_malloc(total).
//    3. Zero the returned memory with memset.
//
void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0|| size == 0) {
        return nullptr;
    }

    // Get total number of bytes needed for allocated
    size_t total = size * nmemb;

    // Use malloc like regular
    void* p = my_malloc(total);

    // Zero all bytes in the memory
    memset(p, 0, total);

    return p;
}

// =============================================================================
//  heap_dump  (diagnostic — implement this early, it will save you time)
// =============================================================================
//
//  Walk every physical block on the heap from heap_start to heap_end and
//  print its address, total size, and whether it is free or allocated.
//
//  Example output:
//    [  0] @0x55a3b0  size=64   ALLOC
//    [  1] @0x55a3f0  size=128  FREE
//
void heap_dump() {
    auto* current = static_cast<Block *>(heap_start);
    int index = 0;

    while (static_cast<void *>(current) < heap_end) {
        printf("[%3d] @%p  size=%-6zu  %s\n",
               index,
               static_cast<void *>(current),
               current->size,
               current->is_free ? "FREE" : "ALLOC");

        current = reinterpret_cast<Block *>(reinterpret_cast<char *>(current) + current->size);
        index++;
    }
}

// =============================================================================
//  heap_check  (diagnostic — used by several tests)
// =============================================================================
//
//  Walk the heap and verify your invariants.  Return true if all pass.
//  Print a message for each violation you detect.
//
//  Suggested checks:
//    1. Every block's size > 0 and is a multiple of ALIGNMENT.
//    2. No two physically adjacent blocks are both free (coalescing invariant).
//    3. Every block in your free list is actually marked free.
//
bool heap_check() {
    auto* current = static_cast<Block *>(heap_start);

    while (static_cast<void *>(current) < heap_end) {
        if (current->size <= 0 && current != free_list) {
            return false;
        }
        if (current->size % 16 != 0) {
            return false;
        }

        current = reinterpret_cast<Block *>(reinterpret_cast<char *>(current) + current->size);
    }

    Block *p = free_list->next;

    while (p != nullptr) {
        if (p->is_free != true) {
            return false;
        }

        p = p->next;
    }

    return true;
}
