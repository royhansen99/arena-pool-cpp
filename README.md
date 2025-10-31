# arena-pool-cpp

A very fast (zero-overhead) memory allocator with an arena/bump-allocator,  
and a pool-allocator.  
Single header c++11 library.

__Arena allocator__  
Essentially a bump-allocator with a pre-allocated  
amount of memory reserved up-front only once. As you allocate portions  
inside the arena-memory it will simply bump a pointer offset.  
Clearing/resetting the arena will simply set the offset back to 0.  
As a result of these things, allocations and clearing an arena is extremely  
fast with zero-overhead. Memory is freed when the class-destructor is called.  
Nested arenas is also supported!

__Pool allocator__  
A contiguous object pool using a singly-linked intrusive  
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

### Benchmarks

__Smaller numbers is better!__

```
Benchmarking 10000000 int allocations with a single cheap mass-dealloc/reset
Arena                   alloc:   0.55 ns  dealloc:   0.00 ns
Pool (Arena)            alloc:   0.62 ns  dealloc:   0.06 ns
Pool (malloc)           alloc:   0.58 ns  dealloc:   0.05 ns
std::vector (reserve()) alloc:   0.89 ns  dealloc:   0.00 ns
std::vector (dynamic)   alloc:   1.50 ns  dealloc:   0.00 ns
std::list               alloc:  11.44 ns  dealloc:  11.13 ns
```

```
Benchmarking 100000 int allocations with individual expensive dealloc
Arena                   (individual dealloc not supported)
Pool (Arena)            alloc:   0.36 ns  dealloc:   0.03 ns
Pool (malloc)           alloc:   0.33 ns  dealloc:   0.03 ns
std::vector (reserve()) alloc:   0.73 ns  dealloc: 3360.98 ns
std::vector (dynamic)   alloc:   1.70 ns  dealloc: 3370.51 ns
std::list               alloc:   9.33 ns  dealloc:  11.37 ns
```

### API Documentation

__Arena__:  
(Destructor will free allocated memory, unless an arena is a child  
in which case the underlying memory belongs to a parent, and will  
be freed by the parent instead)  
- `Arena Arena(i)` // Construct a new `Arena` instance of `i` bytes.   
- `Arena Arena(parent, i) // Construct a new child `Arena` which is  
   nested inside `parent`, child will be `i` bytes.  
- `T* <T>allocate(i) // Allocate a chunk inside the arena with  
  size: sizeof(T) * i  
- `T* <T>allocate_new(args...) // Allocate a single item of type  
  `T`, if it is a class `args` should be constructor parameters.  
  Size: sizeof(T)  
- `void* `allocate_raw(i)` // Allocate a raw chunk of `i` bytes/chars.  
- `bool resize(i)` // Resize the arena to `i` bytes. If this isn't a  
  child arena it will free+malloc a new buffer, otherwise it will  
  grab a new allocation from it's parent without freeing the previous  
  allocation. So in general you want to avoid resizing child arenas  
  because it leads to wasted memory. Resizing will reset/clear data
  in the pool.
- `void reset()` // Reset offset to 0, freeing up all bytes in the arena
  for re-use.  
- `size_t size()` // Get the total bytes/chars allocated in the arena.  
- `size_t used()` // Get the total bytes/chars used in the arena.  

__Pool__:  
(Destructor will free allocated memory when pool owns it's own memory and  
does not use an arena, otherwise if using an arena the arena will take care  
of freeing memory in which case the underlying memory belongs to a parent,  
and will be freed by the parent instead)  
- `Pool<T> Pool(i)` // Construct a new `Pool` of type `T` with a count  
  of `i`, allocated with malloc/free, allocation size: sizeof(T) * `i`  
- `Pool<T> Pool(arena, i)` // Construct a new `Pool` of type `T` with a count  
  of `i`, allocated in `arena`, allocation size: sizeof(T) * `i`  
- `T* allocate()` // Grab a single allocation from the pool for assignment. 
- `T* allocate_new(args...)` // Grab a single allocation from the pool, if it  
  is a class it will use `args` as constructor parameters.  
- `void deallocate(allocate_ptr)`// Release a single allocation by providing  
  a pointer that was received from a previous `allocate()`/`allocate_new()` call.  
- `void reset()` // Release all allocations, freeing up all usage for new  
  allocations in the pool.  
- `bool grow(i)` // Grow the pool by a count of `i` (on top of the current size).  
- `size_t size()` // Get the total count (of type `T`) allocated in the pool.   
- `size_t used()` // Get the used count (of type `T`)  

In general, if allocate* calls fail, they will return `nullptr`, which  
means you can check for that to see if it has failed. A nullptr usually  
means we are out of space.

For grow/resize calls we return true/false to indicate if it was successful  
or not. If it is unsuccessful the Arena/Pool remains intact without  
growing (the Arena is not reset when grow fails).
