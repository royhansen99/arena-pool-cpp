#include "./Arena.h"
#include <iostream>

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

  SArray<int> array(arena, 3);
  // Ommit the arena parameter to simply use malloc
  // instead.
  // SArray<int> array(3);

  array.push(1);
  array.push(2);
  array.push(3);

  *array.at(0); // 1
  *array[0]; // 1
  *array.at(1); // 2 
  *array[1]; // 2 

  array.pop(); // Remove last item.
  array.at(2); // nullptr
  array[2]; // nullptr

  array.erase(0); // Remove item at location 0.
  array.at(0); // nullptr
  array[0]; // nullptr

  array.fill(100); // Fill item into first available
                   // slot, starting from beginning.

  array.at(0); // 100
  array[0]; // 100

  array.push(400);

  array.erase(1);

  array.compact(); // Cover any empty slots by moving forward.
  *array[0]; // 100
  *array[1]; // 400
  *array[2]; // nullptr

  array.resize(6); // Increase size from 3 to 6.

  // Iterate through items in array, will automatically
  // skip empty slots.
  for(auto &it : array) {
    std::cout << "Value: " << it << "\n";
  }

  // When the parent `arena` goes out of scope,
  // the destructor will free() the allocated
  // memory.
  // Since everything in this example is using this
  // arena, it is all freed when the arena is freed. 

  return 0;
}
