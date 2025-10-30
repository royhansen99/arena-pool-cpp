#include "./Arena.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  std::cout << "Running Pool tests...\n";

  // ------------------------------------------------------------------
  // Basic allocation bounds
  // ------------------------------------------------------------------
  {
    Pool<int> pool(3);
    assert(pool.size() == 3);
    assert(pool.used() == 0);

    int* a = pool.allocate_new(111);
    int* b = pool.allocate_new(222);
    int* c = pool.allocate_new(333);
    assert(a && b && c);
    assert(*a == 111 && *b == 222 && *c == 333);
    assert(pool.used() == 3);

    assert(pool.allocate() == nullptr);  // overflow -> nullptr
    assert(pool.used() == 3);
  }

  // ------------------------------------------------------------------
  // Deallocation + reuse (no overflow)
  // ------------------------------------------------------------------
  {
    Pool<int> pool(2);
    int* p1 = pool.allocate_new(1);
    int* p2 = pool.allocate_new(2);
    pool.deallocate(p1);
    pool.deallocate(p2);

    int* p3 = pool.allocate_new(3);
    int* p4 = pool.allocate_new(4);
    assert(p3 && p4);  // reuse works
  }

  // ------------------------------------------------------------------
  // grow() and reset() correctness + no corruption
  // ------------------------------------------------------------------
  {
    Pool<int> pool(1);
    int* p = pool.allocate_new(42);
    assert(p);

    assert(pool.grow(2));  // grow by 2
    assert(pool.size() == 3);

    int* q = pool.allocate_new(99);
    int* r = pool.allocate_new(100);
    assert(q && r);
    assert(pool.used() == 3);
    assert(pool.allocate() == nullptr); // overflow -> nullptr

    pool.deallocate(q);
    pool.deallocate(r);
    pool.deallocate(p);
    assert(pool.used() == 0);  // all back in free list

    pool.allocate_new(444);
    pool.allocate_new(333);
    assert(pool.used() == 2);

    pool.reset();
    assert(pool.used() == 0);
    assert(
      pool.allocate_new(9) && // ok
      pool.allocate_new(6) && // ok
      pool.allocate_new(4) && // ok
      !pool.allocate_new(4) // overflow -> nullptr
    );
  }

  // ------------------------------------------------------------------
  // Arena backend + grow
  // ------------------------------------------------------------------
  {
    Arena arena(1024 * 10);
    Pool<int> pool(arena, 2);
    assert(pool.size() == 2);

    int* a = pool.allocate_new(1);
    int* b = pool.allocate_new(2);
    assert(a && b);

    assert(pool.grow(3));
    assert(pool.size() == 5);

    assert(pool.allocate_new(3));
  }

  // ------------------------------------------------------------------
  // nullptr safety
  // ------------------------------------------------------------------
  {
    Pool<int> pool(1);
    pool.deallocate(nullptr);  // must not crash
    int* p = pool.allocate_new(3);
    pool.deallocate(p);
    assert(p);
    pool.deallocate(p);
    pool.deallocate(nullptr);
  }

  // ------------------------------------------------------------------
  // 100K alloc/dealloc/grow/reset cycles
  // ------------------------------------------------------------------
  {
    Pool<int> pool(1000);
    int* ptrs[1000] = {nullptr};

    for (int cycle = 0; cycle < 100; ++cycle) {
        // Fill half
        for (int i = 0; i < 500; ++i)
            ptrs[i] = pool.allocate_new(i);

        // Grow mid-cycle
        if (cycle == 30 || cycle == 60)
            assert(pool.grow(500));

        // Deallocate in reverse
        for (int i = 499; i >= 0; --i)
            if (ptrs[i]) pool.deallocate(ptrs[i]), ptrs[i] = nullptr;

        // Full reset every 25 cycles
        if (cycle % 25 == 0) {
            pool.reset();
            assert(pool.used() == 0);
        }
    }

    assert(pool.used() == 0);
  }
}
