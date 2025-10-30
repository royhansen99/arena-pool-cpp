#include "./Arena.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  std::cout << "Running Arena tests...\n";

  // ------------------------------------------------------------------
  // Basic allocation bounds
  // ------------------------------------------------------------------
  {
    size_t three_ints = sizeof(int) * 3;
    Arena arena(three_ints);
    assert(arena.size() == three_ints);
    assert(arena.used() == 0);

    int* a = arena.allocate_new<int>(111);
    int* b = arena.allocate_new<int>(222);
    int* c = arena.allocate_new<int>(333);
    assert(a && b && c);
    assert(*a == 111 && *b == 222 && *c == 333);
    assert(arena.used() == three_ints);

    assert(arena.allocate_new<int>(50) == nullptr);  // overflow -> nullptr
    assert(arena.used() == three_ints);

    arena.resize(500); // Resize and clear arena.
    assert(arena.size() == 500 && arena.used() == 0);

    char* name = arena.allocate<char>(20);
    strcpy(name, "John Doe");

    char* country = arena.allocate<char>(20);
    strcpy(country, "England");

    assert(
      strcmp(country, "England") == 0 &&
      strcmp(name, "John Doe") == 0 &&
      arena.used() == sizeof(char) * 40
    );
  }

  // ------------------------------------------------------------------
  // Child arena 
  // ------------------------------------------------------------------
  {
    Arena arena(512);
    Arena child_arena(arena, 256);

    assert(
      child_arena.size() == 256 &&
      child_arena.used() == 0 &&
      arena.size() == 512 &&
      arena.used() == 256
    );

    int* num = child_arena.allocate_new<int>(100);
    assert(
      *num == 100 &&
      child_arena.used() == sizeof(int)
    );

    // Resize (and reset) child arena to a smaller size.
    child_arena.resize(100);

    // Resizing the child arena simply abandoned the old
    // buffer without freeing it, and allocated a brand
    // new buffer with the new size.
    assert(
      child_arena.size() == 100 &&
      arena.used() == (256 + 100)
    );

    // Resizing should fail because the parent does not
    // have enough space.
    assert(
      child_arena.resize(300) == false &&
      child_arena.size() == 100 &&
      arena.used() == (256 + 100)
    );
  }

  // ------------------------------------------------------------------
  // 500 alloc/reset cycles
  // ------------------------------------------------------------------
  {
    Arena arena(256);
    Arena child_arena(arena, 128);

    for(int i = 0; i < 500; i++) {
      char* a = child_arena.allocate<char>(20);
      strcpy(a, "testing something!");

      char* b = child_arena.allocate<char>(20);
      strcpy(b, "something else..");

      int* c = child_arena.allocate<int>(10);
      c[0] = 22;
      c[1] = 99;
      c[5] = 55;

      assert(
        strcmp(a, "testing something!") == 0 &&
        strcmp(b, "something else..") == 0 &&
        c[0] == 22 && c[1] == 99 && c[5] == 55 &&
        child_arena.used() == (
          (sizeof(char) * 40) + (sizeof(int) * 10) 
        ) && 
        arena.used() == 128 &&
        arena.size() == 256 
      );

      child_arena.reset();
    }
  }
}
