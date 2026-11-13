// COMPILE: g++ -std=c++11 -Wall -g -fsanitize=address tests/tests_hashmap.cpp

#include "../src/arena.h"
#include "../src/hashmap.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  std::cout << "Running apc::hasmap tests...\n";

  // Dynamic
  {
    apc::hashmap<int, sizeof(int)> map(6);
      
    assert(map.size() == 6 && map.used() == 0);

    int key = 91;
    map.insert(&key, 1);

    int key2 = 93;
    map.insert(&key2, 2);
      
    assert(
      map.used() == 2 &&
      map.size() == 6 
    );
      
    int key3 = 98;
    map.insert(&key3, 3);
      
    int key4 = 1111;
    map.insert(&key4, 4);

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
      map.insert(&i, i);

    int key = 13;
      
    assert(
      map.size() == 16 && // default initial size is 16 if not specified.
      *map.find(&key) == 13
    );
      
    for(int i = 20; i < 40; i++)
      map.insert(&i, i);
      
    assert(map.size() == 16 + (16 * 2)); // size doubled
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
      map.insert(&i, i);

    assert(_arena.used() > used);
  }

  // Static 
  {
    apc::hashmap_fixed<int, sizeof(int), 3> map;
      
    assert(map.size() == 3 && map.used() == 0);

    int key = 91;
    map.insert(&key, 1);

    int key2 = 93;
    map.insert(&key2, 2);
      
    assert(
      map.used() == 2 &&
      map.size() == 3
    );
      
    int key3 = 98;
    map.insert(&key3, 3);
      
    int key4 = 1111;
    assert(
      !map.insert(&key4, 4) && // Full, so not added.
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

  // insert_new
  {
    struct test {
      int foo;
      const char* bar;

      test(int f, const char* b) : foo(f), bar(b) {}
    };

    apc::hashmap<test, sizeof(int)> map;

    int key = 2;
    auto* item = map.insert_new(&key, 100, "Test..");
    auto* find = map.find(&key);

    assert(item != nullptr && item == find &&
      map.used() == 1 && item->foo == 100 &&
      strcmp(item->bar, "Test..") == 0);
  }

  // Iterators
  {
    apc::hashmap<int, sizeof(int)> map;

    for(auto i : {1, 2, 3, 4})
      map.insert(&i, i);

    char num[3];

    {
      std::string result;

      for(auto i : map) {
        snprintf(num, 3, "%d", i);

        if(result != "") result += ",";

        result += num;
      }

      assert(result == "1,2,3,4");
    }

    {
      std::string result;

      for(auto it = map.rbegin(); it != map.rend(); ++it) {
        snprintf(num, 3, "%d", *it);

        if(result != "") result += ",";

        result += num;
      }

      assert(result == "4,3,2,1");
    }

    { // Erase using iterator.
      assert(map.erase(map.rbegin()) &&
        *map.rbegin() == 3 && map.used() == 3);
      assert(map.erase(map.begin()) &&
        *map.begin() == 2 && map.used() == 2);
    }

    map.reset();

    assert(map.used() == 0 && !(map.begin() != nullptr) &&
      !(map.rbegin() != nullptr));
  }

  // Iterators fixed
  {
    apc::hashmap_fixed<int, sizeof(int), 10> map;

    for(auto i : {1, 2, 3, 4})
      map.insert(&i, i);

    char num[3];

    {
      std::string result;

      for(auto i : map) {
        snprintf(num, 3, "%d", i);

        if(result != "") result += ",";

        result += num;
      }

      assert(result == "1,2,3,4");
    }
  }

  // Move semantics
  {
    apc::hashmap<int, sizeof(int)> first;

    for(auto i : {1,2,3})
      first.insert(&i, i);

    int key_one = 1;
    auto* item_one = first.find(&key_one);
    int key_three = 3;
    auto* item_three = first.find(&key_three);

    apc::hashmap<int, sizeof(int)> second;
    second = (std::move(first));

    assert(second.used() == 3 && first.used() == 0 && first.size() == 0 &&
      second.find(&key_one) == item_one &&
      second.find(&key_three) == item_three &&
      *item_one == 1 && *item_three == 3);

    apc::hashmap<int, sizeof(int)> third(std::move(second));

    assert(third.used() == 3 && second.used() == 0 && second.size() == 0 &&
      third.find(&key_one) == item_one &&
      third.find(&key_three) == item_three);
  }

  // Copy semantics
  {
    apc::hashmap_fixed<int, sizeof(int), 10> first;

    for(auto i : {1,2,3})
      first.insert(&i, i);

    apc::hashmap_fixed<int, sizeof(int), 10> second(first);

    assert(first.used() == second.used() &&
      *first.begin() == *second.begin() &&
      *first.rbegin() == *second.rbegin() &&
      &*first.begin() != &*second.begin() &&
      &*first.rbegin() != &*second.rbegin()); 

    apc::hashmap<int, sizeof(int)> third;
    third = second;

    assert(second.used() == third.used() &&
      *second.begin() == *third.begin() &&
      *second.rbegin() == *third.rbegin() &&
      &*second.begin() != &*third.begin() &&
      &*second.rbegin() != &*third.rbegin()); 


    apc::hashmap_fixed<int, sizeof(int), 10> fourth(third);

    assert(third.used() == fourth.used() &&
      *third.begin() == *fourth.begin() &&
      *third.rbegin() == *fourth.rbegin() &&
      &*third.begin() != &*fourth.begin() &&
      &*third.rbegin() != &*fourth.rbegin()); 

    apc::hashmap_fixed<int, sizeof(int), 10> fifth;
    fifth = third;

    assert(third.used() == fifth.used() &&
      *third.begin() == *fifth.begin() &&
      *third.rbegin() == *fifth.rbegin() &&
      &*third.begin() != &*fifth.begin() &&
      &*third.rbegin() != &*fifth.rbegin()); 
  }

  return 0;
}
