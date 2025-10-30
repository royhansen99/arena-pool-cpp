# arena-pool-cpp

A very fast (zero-overhead) memory allocator with an arena/bump-allocator,  
and a pool-allocator.  
Single header c++11 library.

`Arena` allocator is essentially a bump-allocator with a pre-allocated  
amount of memory reserved up-front only once. As you allocate portions  
inside the arena-memory it will simply bump a pointer offset.  
Clearing/resetting the arena will simply set the offset back to 0.  
As a result of these things, allocations and clearing an arena is extremely  
fast with zero-overhead. Memory is freed when the class-destructor is called.  
Nested arenas is also supported!

`Pool` allocator is a contiguous object pool using a singly-linked intrusive  
free list. Each PoolItem<T> contains a next pointer and the user object T.  
Allocation pops from the free list; deallocation pushes to the head.  
Multiple fixed-size buffers are managed in a growable array of metadata  
(PoolBuffer<T>). The free list spans all buffers, enabling O(1) operations  
and full reset/recycling.  
This allocator works by either using an `Arena`, or by managing it's own  
memory using malloc/free.  
If the pool manages it's own memory, free happens when the class-destructor  
is called.

### Usage

Simply drop the single header `Arena.h` file directly in to your project. 

```cpp
/* (From ./example.cpp in this repo) */
#include "./Arena.h"

struct Foo {
    char value[20];
    int number;
};

int main() {
  // Allocate a 1024 byte arena.
  Arena arena(1024);

  arena.size(); // 1024 bytes

  // Allocate a 30-char chunk from the arena.
  char* str = arena.allocate<char>(30);

  arena.used(); // 30 bytes (of 1024 bytes)

  // Allocate a pool of 5 Foo's from the arena.
  Pool<Foo> foo_pool(arena, 5); 

  // You could also create an independent pool
  // that uses malloc/free instead of an arena,
  // by only providing a single size argument.
  // The pool will automatically free memory
  // when it goes out of scope.
  // Pool<Foo> foo_pool(5); 

  arena.used(); // 208 bytes (of 1024 bytes)

  // Allocate some items from the pool.
  Foo* foo1 = foo_pool.allocate();
  Foo* foo2 = foo_pool.allocate();
  Foo* foo3 = foo_pool.allocate();

  // Deallocate/remove one item.
  foo_pool.deallocate(foo3);

  foo_pool.used(); // 2 x Foo (of 5)
  foo_pool.size(); // 5

  // Grow the pool by allocating space for 10
  // additional Foo's from the underlying arena.
  // This will be added as a separate buffer to
  // the pool, and is not contigous with the
  // initial allocation of 5.
  foo_pool.grow(10);

  foo_pool.size(); // 15 

  arena.used(); // 560 bytes (of 1024 bytes)

  // Allocate a 400 byte nested arena.
  Arena child_arena(arena, 400);

  child_arena.size(); // 400 bytes
  child_arena.used(); // 0 bytes (of 400 bytes)
  arena.used(); // 960 bytes (of 1024 bytes)
   
  // Clear all allocations.
  // Since this arena also contained `foo_pool`,
  // `str` and `child_arena`, they are also cleared
  // by simply resetting the parent arena.
  // Reset is only useful if you are going to re-use
  // the arena for new allocations, otherwise you
  // could simply rely on the automatic free when the
  // arena goes out of scope.
  arena.reset(); 
  arena.used(); // 0 bytes (of 1024 bytes)

  // When the parent `arena` goes out of scope,
  // the destructor will free() the allocated
  // memory.
  // Since everything in this example is using this
  // arena, it is all freed when the arena is freed. 

  return 0;
}
```
