# arena-pool-cpp

A very fast memory allocator with an arena/bump-allocator, a pool-allocator,  
and a vector-like array-allocator. Also includes a string implementation.  
Single header c++11 library.

Documentation for the string implementation is in a separate file: [STRING.md](STRING.md) 

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

__apc::vector allocator__  
Works in much the same way as `std::vector`.  
Will pre-allocate a specified size, with the ability to shrink/grow.  
Shrink is a manual operation.  
This allocator works by either using an `Arena`, or by managing it's own  
memory using malloc/free.  

### Usage

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
Arena                   alloc:   0.72 ns  dealloc:   0.00 ns
Pool (Arena)            alloc:   1.87 ns  dealloc:   0.70 ns
Pool (malloc)           alloc:   1.61 ns  dealloc:   0.51 ns
apc::vector (reserve)   alloc:   0.84 ns  dealloc:   0.00 ns
apc::vector (dynamic)   alloc:   0.96 ns  dealloc:   0.00 ns
std::vector (reserve()) alloc:   0.52 ns  dealloc:   0.00 ns
std::vector (dynamic)   alloc:   1.42 ns  dealloc:   0.00 ns
std::list               alloc:  10.84 ns  dealloc:  11.52 ns
```

```
Benchmarking 100000 int allocations with individual expensive dealloc
Arena                   (individual dealloc not supported)
Pool (Arena)            alloc:   0.73 ns  dealloc:   0.45 ns
Pool (malloc)           alloc:   0.68 ns  dealloc:   0.40 ns
apc::vector (reserve)   alloc:   0.88 ns  dealloc: 3358.54 ns
apc::vector (dynamic)   alloc:   1.22 ns  dealloc: 3354.82 ns
std::vector (reserve()) alloc:   0.73 ns  dealloc: 3370.19 ns
std::vector (dynamic)   alloc:   2.63 ns  dealloc: 3364.49 ns
std::list               alloc:   9.63 ns  dealloc:  11.91 ns
```

### API Documentation

__apc::arena__:  
(Destructor will free allocated memory, unless an arena is a child  
in which case the underlying memory belongs to a parent, and will  
be freed by the parent instead)  
| Function Name                          | Description                                                                                     |
|----------------------------------------|-------------------------------------------------------------------------------------------------|
| `apc::arena arena(i)`                       | Construct a new arena instance of `i` bytes.                                                |
| `apc::arena arena(parent, i)`               | Construct a new `child` arena which is nested inside `parent`, child will be `i` bytes.       |
| `T* allocate_size<T>(i)`                   | Allocate a chunk inside the arena with size: sizeof(T) * i                                   |
| `T* allocate<T>(item)`                   | Allocate space for `T` and copy `item` into it.                                   |
| `T* allocate_new<T>(args...)`         | Allocate a single item of type `T`, if it is a class `args` should be constructor parameters. Size: sizeof(T) |
| `void* allocate_raw(i)`                | Allocate a raw chunk of `i` bytes/chars.                                                      |
| `bool resize(i)`                       | Resize the arena to `i` bytes. If this isn't a child arena it will free+malloc a new buffer, otherwise it will grab a new allocation from its parent without freeing the previous allocation. So in general you want to avoid resizing child arenas because it leads to wasted memory. Resizing will reset/clear data in the pool. |
| `void reset()`                         | Reset offset to 0, freeing up all bytes in the arena for re-use.                              |
| `size_t size()`                        | Get the total bytes/chars allocated in the arena.                                             |
| `size_t used()`                        | Get the total bytes/chars used in the arena.                                                 |

__apc::pool__:  
(Destructor will free allocated memory when pool owns it's own memory and  
does not use an arena, otherwise if using an arena the arena will take care  
of freeing memory in which case the underlying memory belongs to a parent,  
and will be freed by the parent instead)  
| Function Name                     | Description                                                                                     |
|-----------------------------------|-------------------------------------------------------------------------------------------------|
| `apc::pool<T> pool(i)`                 | Construct a new `pool` of type `T` with a count of `i`, allocated with malloc/free, allocation size: sizeof(T) * `i` |
| `apc::pool<T> pool(arena, i)`          | Construct a new `pool` of type `T` with a count of `i`, allocated in `arena`, allocation size: sizeof(T) * `i` |
| `T* allocate(item)`           | Grab a single allocation from the pool and copy item into it.                                  |
| `T* allocate_new(args...)`        | Grab a single allocation from the pool, if it is a class it will use `args` as constructor parameters. |
| `void deallocate(allocate_ptr)`   | Release a single allocation by providing a pointer that was received from a previous `allocate()`/`allocate_new()` call. Will also call destructor if non-trivial T. |
| `void reset()`                    | Release all allocations, freeing up all usage for new allocations in the pool. Will also call destructor on all items if non-trivial T. |
| `bool grow(i)`                    | Grow the pool by a count of `i` (on top of the current size).                                 |
| `size_t size()`                   | Get the total count (of type `T`) allocated in the pool.                                      |
| `size_t used()`                   | Get the used count (of type `T`)                                                               |

In general, if allocate* calls fail, they will return `nullptr`, which  
means you can check for that to see if it has failed. A nullptr usually  
means we are out of space.

For grow/resize calls we return true/false to indicate if it was successful  
or not. If it is unsuccessful the Arena/Pool remains intact without  
growing (the Arena is not reset when grow fails).


__apc::vector__:  
(Destructor will free allocated memory when pool owns it's own memory and  
does not use an arena, otherwise if using an arena the arena will take care  
of freeing memory in which case the underlying memory belongs to a parent,  
and will be freed by the parent instead)  
| Function Name                     | Description                                                                                          |
|-----------------------------------|------------------------------------------------------------------------------------------------------|
| `apc::vector<T> vector(i)`             | Construct a new `vector` of type `T` with a count of `i`, allocated with malloc/free.              |
| `T& at(pos)`                      | Get item at position. Will return `nullptr` if empty slot.                                         |
| `T& operator[]`                   | Array accessor s_array[2], works the same as `at(2)`.                                             |
| `T* first()`                      | Get the first item in the array. `nullptr` if array is empty.                                     |
| `T* last()`                       | Get the last item in the array. `nullptr` if array is empty.                                      |
| `T* push(item)`                   | Push new item to end of array. `nullptr` if full and automatic grow fails.       |
| `T* push_new(...args)`            | Construct new item at end of array, by directly specifying constructor params in this method.       |
| `T* replace(pos, item)`            | Insert `item` to array by replacing with item in `pos`. Slower than `push()`.                          |
| `T* replace_new(pos, ...args)`     | Same as replace(), but will construct/new with args.                                                  |
| `T* insert(pos, item)`     | Insert `item` before `pos`                                                  |
| `T* insert(pos, size_t count, item)`     | Insert `item`, `x` times, before `pos`.                                                  |
| `T* insert_new(pos, ..args)`     | Same as insert(),but will construct/new with args.                          |
| `void pop()`                      | Remove item at the end of array. Will also call destruct if non-trivial T.                         |
| `void erase(pos)`                 | Remove item at specific position. Will leave an empty slot.                                        |
| `void erase_ptr(ptr)`             | Same as `erase()`, but you provide a pointer to an item instead of a position.                     |
| `void reset()`                    | Clear/empty all items. Will also call destruct on all items if non-trivial T.                      |
| `bool resize(size)`               | Change the allocated size of the array. Will run `compact()` before resizing.                      |
| `bool shrink_to_fit()`            | Shrink size to fit usage(), uses resize() to achieve this. If usage() == 0, will resize to 1.      |
| `size_t size()`                   | Get the total count (of type `T`) allocated in the pool.                                           |
| `size_t used()`                   | Get the used count (of type `T`).                                                                  |
| `bool empty()`                    | Check if empty.                                                                                    |


__apc::vector_fixed__: (Fixed size vector)  
Mostly the same as apc::vector, except that it is fixed size.  

`apc::vector_fixed<T, size>`  

These method are not available on `apc::vector_fixed`, since it is fixed-size:  
- resize()  
- shrink\_to\_fit()  
