// COMPILE: g++ -std=c++11 -Wall -fsanitize=address tests/tests_hashmap.cpp

#include "../src/arena.h"
#include "../src/hashmap.h"
#include <cassert>
#include <cstring>

int main() {
  // Dynamic
  {
    apc::hashmap<int, sizeof(int)> map(6);
      
    assert(map.size() == 6 && map.used() == 0);

    int key = 91;
    map.insert(1, &key);

    int key2 = 93;
    map.insert(2, &key2);
      
    assert(
      map.used() == 2 &&
      map.size() == 6 
    );
      
    int key3 = 98;
    map.insert(3, &key3);
      
    int key4 = 1111;
    map.insert(4, &key4);

    assert(
      map.size() == 6 
    );

    int not_found = 5;

    assert(
      *map.find(&key) == 1 &&
      *map.find(&key3) == 3 &&
      map.find(&not_found) == nullptr
    );

    assert(
      map.erase(&key) == true &&
      map.find(&key) == nullptr
    );

    map.reset();

    assert(map.used() == 0);
  }
    
  // Dynamically grow
  {
    apc::hashmap<int, sizeof(int)> map;

    assert(map.size() == 0);

    for(int i = 0; i < 15; i++)
      map.insert(i, &i);

    int key = 13;
      
    assert(
      map.size() == 16 && // default initial size is 16 if not specified.
      *map.find(&key) == 13
    );
      
    for(int i = 20; i < 40; i++)
      map.insert(i, &i);
      
    assert(map.size() == 32); // size doubled
  }

  // Arean
  {
    apc::arena _arena(1024);

    assert(_arena.used() == 0);

    apc::hashmap<int, sizeof(int)> map(_arena);

    size_t used = _arena.used();

    assert(
      used > 10 &&
      map.size() > 0
    );
    
    for(int i = 0; i < 32; i++)
      map.insert(i, &i);

    assert(_arena.used() > used);
  }

  // Static 
  {
    apc::hashmap_fixed<int, sizeof(int), 3> map;
      
    assert(map.size() == 3 && map.used() == 0);

    int key = 91;
    map.insert(1, &key);

    int key2 = 93;
    map.insert(2, &key2);
      
    assert(
      map.used() == 2 &&
      map.size() == 3
    );
      
    int key3 = 98;
    map.insert(3, &key3);
      
    int key4 = 1111;
    assert(
      !map.insert(4, &key4) && // Full, so not added.
      map.find(&key4) == nullptr
    );

    int not_found = 5;

    assert(
      *map.find(&key) == 1 &&
      *map.find(&key3) == 3 &&
      map.find(&not_found) == nullptr
    );

    assert(
      map.erase(&key) == true &&
      map.find(&key) == nullptr
    );

    map.reset();

    assert(map.used() == 0);
  }

  return 0;
}
