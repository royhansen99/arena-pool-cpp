# arena-pool-cpp

A very fast memory allocator library with an arena/bump-allocator,  
a pool-allocator, a hashmap-allocator,  and a vector-like  
array-allocator. Also includes a string implementation.  
Single header c++11 library.  

To avoid virtual lookup overhead, this library does not use  
abstract-classes/virtual-functions. Because of this we rely on  
macros. This is a low-level library, so we want maximum speed. 

__apc::arena allocator__  
Essentially a bump-allocator with a pre-allocated  
amount of memory reserved up-front only once. As you allocate portions  
inside the arena-memory it will simply bump a pointer offset.  
Clearing/resetting the arena will simply set the offset back to 0.  
As a result of these things, allocations and clearing an arena is extremely  
fast with zero-overhead. Memory is freed when the class-destructor is called.  
Nested arenas is also supported!

__apc::pool allocator__  
A contiguous object pool using a combined doubly/singly-linked free/used list.  
Each PoolItem<T> contains a prev/next pointer and the user object T.  
Allocation pops from the free list; deallocation pushes to the head.  
Multiple fixed-size buffers are managed in a growable array of metadata  
(PoolBuffer<T>). The free list spans all buffers, enabling O(1) operations  
and full reset/recycling.  
This allocator works by either using an `Arena`, or by managing it's own  
memory using malloc/free.  
If the pool manages it's own memory, free happens when the class-destructor  
is called.

__apc::hashmap allocator__  
A hashmap implementation which uses apc::vector for indexes, and apc::pool  
for storing items/content.   
Indexes are organized into doubly linked lists, to handle hash collisions  
when multiple items share the same index.  
Uses rapidhash for hashing keys.  
Key types are very flexible because they are void pointers of N size  
controlled by you, so can even be structs, as long as you make sure padding  
is zeroed.    
This allocator also supports apc::arena.

__apc::vector allocator__  
Works in much the same way as `std::vector`.  
Will pre-allocate a specified size, with the ability to shrink/grow.  
Shrink is a manual operation.  
This allocator works by either using an `Arena`, or by managing it's own  
memory using malloc/free.  

__apc::string allocator__  
Works in much the same was as `std::string`.  
This allocator also supports apc::arena.  
Documentation for the string implementation is in a separate file: [STRING.md](STRING.md) 

### Usage

Have a look in the "tests/" folder for more usage examples.  

Simply drop the single header `Arena.h` file directly in to your project. 

```cpp
/* (From ./example.cpp in this repo) */
#include "../src/arena.h"
#include "../src/vector.h"
#include <iostream>

struct Foo {
    char value[20];
    int number;
};

int main() {
  // Allocate a 1024 byte arena.
  apc::arena arena(1024);

  arena.size(); // 1024 bytes

  // Allocate a 30-char chunk from the arena.
  char* str = arena.allocate_size<char>(30);

  arena.used(); // 30 bytes (of 1024 bytes)

  // Allocate a pool of 5 Foo's from the arena.
  apc::pool<Foo> foo_pool(arena, 5); 

  // You could also create an independent pool
  // that uses malloc/free instead of an arena,
  // by only providing a single size argument.
  // The pool will automatically free memory
  // when it goes out of scope.
  // apc::pool<Foo> foo_pool(5); 

  arena.used(); // 208 bytes (of 1024 bytes)

  // Allocate some items from the pool.
  Foo* foo1 = foo_pool.allocate(Foo{"Test1", 1});
  Foo* foo2 = foo_pool.allocate(Foo{"Test2", 2});
  Foo* foo3 = foo_pool.allocate(Foo{"Test3", 3});

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
  apc::arena child_arena(arena, 400);

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

  apc::vector<int> array(arena, 3);
  // Ommit the arena parameter to simply use malloc
  // instead.
  // apc::vector<int> array(3);

  array.push(1);
  array.push(2);
  array.push(3);

  array.at(0); // 1
  array[0]; // 1
  array.at(1); // 2 
  array[1]; // 2 

  array.pop(); // Remove last item.

  array.erase(0); // Remove item at position 0.

  array.push(400);

  array[0]; // 2 
  array[1]; // 400

  array.resize(6); // Increase size from 3 to 6.

  // Iteration 
  for(auto &it : array) {
    std::cout << "Value: " << it << "\n";
  }

  // Reverse iteration
  for (auto it = array.rbegin(); it != array.rend(); ++it) {
    std::cout << "Value: " << *it << "\n";
  }

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
apc::arena              alloc:   0.60 ns  dealloc:   0.00 ns
apc::pool (reserve)     alloc:   3.11 ns  dealloc:   0.79 ns
apc::pool (dynamic)     alloc:   6.26 ns  dealloc:   1.23 ns
apc::vector (reserve)   alloc:   0.72 ns  dealloc:   0.00 ns
apc::vector (dynamic)   alloc:   0.92 ns  dealloc:   0.00 ns
std::vector (reserve)   alloc:   0.47 ns  dealloc:   0.00 ns
std::vector (dynamic)   alloc:   1.08 ns  dealloc:   0.00 ns
std::list               alloc:  10.15 ns  dealloc:  10.69 ns
```

```
Benchmarking 100000 int allocations with individual expensive dealloc
apc::arena              (individual dealloc not supported)
apc::pool (reserve)     alloc:   2.18 ns  dealloc:   0.39 ns
apc::pool (dynamic)     alloc:   5.46 ns  dealloc:   0.79 ns
apc::vector (reserve)   alloc:   0.87 ns  dealloc: 3340.02 ns
apc::vector (dynamic)   alloc:   1.02 ns  dealloc: 3347.29 ns
std::vector (reserve)   alloc:   0.96 ns  dealloc: 3350.97 ns
std::vector (dynamic)   alloc:   1.66 ns  dealloc: 3345.05 ns
std::list               alloc:   9.17 ns  dealloc:  11.54 ns
```

### Build tests 

// MacOS
> cmake -S . -B build -G Xcode

// Windows
> cmake -S . -B build -G "Visual Studio 17 2022"

Open the appropriate project-file in Xcode or visual studio, choose the target executable to build/run.
