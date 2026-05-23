// =============================================================================
// tests.cpp  —  Do NOT modify this file.
// =============================================================================
// Run with:  make run
//
// Every test calls heap_check() at the end to verify heap consistency.
// All 15 tests must PASS for full marks.
// =============================================================================

#include "my_malloc.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <random>

// ── harness ───────────────────────────────────────────────────────────────────
static int g_pass = 0, g_fail = 0;

#define TEST(name)                                                              \
    static void test_##name();                                                  \
    struct Runner_##name {                                                      \
        Runner_##name() {                                                       \
            bool ok = true;                                                     \
            try { test_##name(); }                                              \
            catch (...) { ok = false; }                                         \
            if (ok) { printf("  PASS  %s\n", #name); ++g_pass; }               \
            else    { printf("  FAIL  %s\n", #name); ++g_fail; }               \
        }                                                                       \
    } runner_##name;                                                            \
    static void test_##name()

#define EXPECT(expr)                                                            \
    do { if (!(expr)) {                                                         \
        printf("       EXPECT failed: %s  (%s:%d)\n",#expr,__FILE__,__LINE__); \
        throw 0;                                                                \
    }} while(0)

// ─────────────────────────────────────────────────────────────────────────────

// T01: my_malloc returns a non-null pointer for a basic request
TEST(basic_alloc) {
    void* p = my_malloc(64);
    EXPECT(p != nullptr);
    my_free(p);
    EXPECT(heap_check());
}

// T02: every returned pointer is 16-byte aligned
TEST(alignment) {
    for (size_t sz : {1u, 7u, 8u, 15u, 16u, 17u, 63u, 64u, 255u, 1000u}) {
        void* p = my_malloc(sz);
        EXPECT(p != nullptr);
        EXPECT((reinterpret_cast<uintptr_t>(p) % ALIGNMENT) == 0);
        my_free(p);
    }
    EXPECT(heap_check());
}

// T03: data written into an allocation is read back correctly
TEST(write_read) {
    const size_t N = 256;
    auto* buf = static_cast<unsigned char*>(my_malloc(N));
    EXPECT(buf != nullptr);
    for (size_t i = 0; i < N; ++i) buf[i] = static_cast<unsigned char>(i);
    for (size_t i = 0; i < N; ++i)
        EXPECT(buf[i] == static_cast<unsigned char>(i));
    my_free(buf);
    EXPECT(heap_check());
}

// T04: my_free(nullptr) must not crash
TEST(free_null) {
    my_free(nullptr);
    EXPECT(heap_check());
}

// T05: my_malloc(0) returns nullptr
TEST(malloc_zero) {
    void* p = my_malloc(0);
    EXPECT(p == nullptr);
}

// T06: multiple live allocations must not overlap
TEST(no_overlap) {
    const int   K  = 10;
    const size_t SZ = 128;
    unsigned char* ptrs[K];
    for (int i = 0; i < K; ++i) {
        ptrs[i] = static_cast<unsigned char*>(my_malloc(SZ));
        EXPECT(ptrs[i] != nullptr);
        memset(ptrs[i], i, SZ);
    }
    for (int i = 0; i < K; ++i)
        for (size_t j = 0; j < SZ; ++j)
            EXPECT(ptrs[i][j] == static_cast<unsigned char>(i));
    for (int i = 0; i < K; ++i) my_free(ptrs[i]);
    EXPECT(heap_check());
}

// T07: freeing adjacent blocks coalesces them into one
TEST(coalescing) {
    const size_t SZ = 64;
    void* a = my_malloc(SZ);
    void* b = my_malloc(SZ);
    void* c = my_malloc(SZ);
    EXPECT(a && b && c);
    my_free(b);
    my_free(a);
    my_free(c);
    EXPECT(heap_check());
    // The merged region must be re-usable as a single larger allocation
    void* big = my_malloc(SZ * 2);
    EXPECT(big != nullptr);
    my_free(big);
    EXPECT(heap_check());
}

// T08: my_realloc grows a buffer and preserves contents
TEST(realloc_grow) {
    char* p = static_cast<char*>(my_malloc(32));
    EXPECT(p != nullptr);
    strcpy(p, "hello");
    p = static_cast<char*>(my_realloc(p, 256));
    EXPECT(p != nullptr);
    EXPECT(strcmp(p, "hello") == 0);
    my_free(p);
    EXPECT(heap_check());
}

// T09: my_realloc(ptr, 0) frees the block and returns nullptr
TEST(realloc_zero) {
    void* p = my_malloc(64);
    EXPECT(p != nullptr);
    void* q = my_realloc(p, 0);
    EXPECT(q == nullptr);
    EXPECT(heap_check());
}

// T10: my_realloc(nullptr, sz) behaves like my_malloc(sz)
TEST(realloc_null) {
    void* p = my_realloc(nullptr, 128);
    EXPECT(p != nullptr);
    EXPECT((reinterpret_cast<uintptr_t>(p) % ALIGNMENT) == 0);
    my_free(p);
    EXPECT(heap_check());
}

// T11: my_calloc returns memory that is fully zeroed
TEST(calloc_zeroed) {
    const size_t N = 100;
    int* arr = static_cast<int*>(my_calloc(N, sizeof(int)));
    EXPECT(arr != nullptr);
    for (size_t i = 0; i < N; ++i) EXPECT(arr[i] == 0);
    my_free(arr);
    EXPECT(heap_check());
}

// T12: allocation larger than the default heap chunk (64 KiB)
TEST(large_alloc) {
    const size_t BIG = 1u << 16;
    auto* p = static_cast<unsigned char*>(my_malloc(BIG));
    EXPECT(p != nullptr);
    memset(p, 0xAB, BIG);
    for (size_t i = 0; i < BIG; ++i) EXPECT(p[i] == 0xAB);
    my_free(p);
    EXPECT(heap_check());
}

// T13: 2 000 random alloc/free operations leave the heap consistent
TEST(stress_random) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> sizes(1, 512);
    std::uniform_int_distribution<int>    action(0, 2);
    std::vector<void*> live;
    live.reserve(128);
    for (int i = 0; i < 2000; ++i) {
        if (action(rng) == 0 || live.empty()) {
            void* p = my_malloc(sizes(rng));
            if (p) live.push_back(p);
        } else {
            std::uniform_int_distribution<size_t> pick(0, live.size()-1);
            size_t idx = pick(rng);
            my_free(live[idx]);
            live.erase(live.begin() + static_cast<ptrdiff_t>(idx));
        }
    }
    for (void* p : live) my_free(p);
    EXPECT(heap_check());
}

// T14: freed memory is reused (heap does not grow monotonically)
TEST(reuse_freed) {
    const size_t SZ = 256;
    void* first = my_malloc(SZ);
    EXPECT(first != nullptr);
    my_free(first);
    void* second = my_malloc(SZ);
    EXPECT(second != nullptr);
    my_free(second);
    EXPECT(heap_check());
}

// T15: splitting a large block produces two independently usable regions
TEST(split_block) {
    void* big = my_malloc(512);
    EXPECT(big != nullptr);
    my_free(big);
    void* a = my_malloc(16);
    void* b = my_malloc(16);
    EXPECT(a != nullptr);
    EXPECT(b != nullptr);
    EXPECT(a != b);
    my_free(a);
    my_free(b);
    EXPECT(heap_check());
}

// ─────────────────────────────────────────────────────────────────────────────
int main() {
    printf("\n══════════════════════════════════════════════\n");
    printf("  my_malloc — test suite\n");
    printf("══════════════════════════════════════════════\n\n");

    printf("\n──────────────────────────────────────────────\n");
    printf("  Results: %d passed, %d failed  (total %d)\n",
           g_pass, g_fail, g_pass + g_fail);
    printf("──────────────────────────────────────────────\n\n");

    heap_dump();

    return g_fail ? 1 : 0;
}
