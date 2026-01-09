// COMPILE: g++ -std=c++11 -g -Wall -fsanitize=address tests_string_arena.cpp

#include "../src/arena.h"
#include "../src/string.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Running String tests...\n";

  // ------------------------------------------------------------------
  // Basic usage
  // ------------------------------------------------------------------
  {
    apc::arena arena(1024);

    apc::str_dynamic<1> str(arena);

    assert(arena.used() == 0);

    str = "Test!";

    assert(arena.used() == 6);
  }
}
