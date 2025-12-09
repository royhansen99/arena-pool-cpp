// COMPILE: g++ -std=c++11 -Wall -fsanitize=address tests_sarray.cpp

#include "./Arena.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Running SArray tests...\n";

  // ------------------------------------------------------------------
  // Basic usage (malloc) 
  // ------------------------------------------------------------------
  {
    SArray<int> arr(4);

    assert(
      arr.size() == 4 &&
      arr.used() == 0 &&
      arr.empty() == true
    );

    arr = {1,2};

    assert(
      arr.used() == 2 &&
      *arr[0] == 1 &&
      *arr[1] == 2
    );

    arr = {};

    auto* p1 = arr.push_new(1);
    auto* p2 = arr.push_new(2);
    int three = 3;
    auto* p3 = arr.push(three);
    int four = 4;
    auto* p4 = arr.push(four);

    // Out of bounds assertions
    assert(
      arr.push(22) == nullptr && // Array full
      arr.fill(22) == nullptr && // Array full
      arr[-1] == nullptr &&
      arr[5] == nullptr &&
      arr.at(-1) == nullptr &&
      arr.at(5) == nullptr
    );

    assert(
      *arr[0] == 1 && *p1 == 1 && 
      *arr[1] == 2 && *p2 == 2 && 
      *arr[2] == 3 && *p3 == 3 && 
      *arr[3] == 4 && *p4 == 4 && 
      *arr.at(0) == 1 &&
      *arr.at(1) == 2 && 
      *arr.at(2) == 3 && 
      *arr.at(3) == 4 && 
      arr.used() == 4 &&
      arr.size() == 4 &&
      arr.empty() == false &&
      *arr.first() == 1 && *arr.last() == 4
    );

    arr.pop();

    assert(
      arr.used() == 3 &&
      arr[3] == nullptr &&
      arr.at(3) == nullptr
    );

    arr.erase(0);

    assert(
      arr.used() == 2 &&
      arr[0] == nullptr &&
      arr.at(0) == nullptr
    );

    assert(
      *arr.fill(9) == 9 && *arr[0] == 9 &&
      arr.used() == 3 &&
      *arr.fill(7) == 7 && *arr[3] == 7 &&
      arr.used() == 4
    );

    arr.erase(1);
    arr.compact();

    assert(
      *arr[0] == 9 &&
      *arr[1] == 3 &&
      *arr[2] == 7 &&
      arr.used() == 3
    );

    arr.resize(6);

    assert(
      arr.size() == 6 &&
      *arr.push(10) == 10 &&
      *arr.push(11) == 11 &&
      *arr.push(12) == 12 &&
      *arr[0] == 9 &&
      *arr[1] == 3 &&
      *arr[2] == 7 &&
      arr[6] == nullptr
    );

    int i = 0;
    for(auto &it : arr) {
      assert(
        (i == 0 && it == 9) || 
        (i == 1 && it == 3) || 
        (i == 2 && it == 7) || 
        (i == 3 && it == 10) || 
        (i == 4 && it == 11) || 
        (i == 5 && it == 12) 
      );

      i++;
    }

    arr.erase_ptr(arr[2]);

    assert(
      arr.used() == 5 &&
      arr[2] == nullptr
    );

    arr.resize(3);

    assert(
      arr.used() == 3 &&
      arr.size() == 3 &&
      *arr[0] == 9 &&
      *arr[1] == 3 &&
      *arr[2] == 10 &&
      arr[3] == nullptr
    );

    arr.replace(1, 900);

    assert(
      *arr[1] == 900
    );

    arr.resize(20);

    assert(
      arr.size() == 20
    );

    arr.shrink_to_fit();

    assert(
      arr.size() == 3
    );

    arr.reset();

    assert(
      arr.used() == 0
    );
  }

  // ------------------------------------------------------------------
  // Constuctor assignment 
  // ------------------------------------------------------------------
  {
    SArray<int> arr(3, {1,2,3});
    
    assert(
      arr.size() == 3 &&
      arr.used() == 3 &&
      *arr[0] == 1 &&
      *arr[1] == 2 &&
      *arr[2] == 3
    );

    Arena arena(32);
    SArray<int> arena_arr(arena, 3, {1,2,3});

    assert(
      arena_arr.size() == 3 &&
      arena_arr.used() == 3 &&
      *arena_arr[0] == 1 &&
      *arena_arr[1] == 2 &&
      *arena_arr[2] == 3
    );

    SArrayFixed<int, 3> fixed({1,2,3});

    assert(
      fixed.size() == 3 &&
      fixed.used() == 3 &&
      *fixed[0] == 1 &&
      *fixed[1] == 2 && 
      *fixed[2] == 3 
    );
  }

  // ------------------------------------------------------------------
  // Iteration 
  // ------------------------------------------------------------------
  {
    SArray<int> arr(6);
    arr.push(0);
    arr.push(1);
    arr.push(2);
    arr.push(3);
    arr.push(4);
    arr.push(5);

    std::string test = "";

    for(const auto &it : arr) {
      test += it + '0';
    }

    assert(test == "012345");

    // Reverse
    test = "";
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
      test += *it + '0';
    }

    assert(test == "543210");

    arr.erase(0);
    arr.erase(3);
    arr.erase(5);

    test = "";
    for(const auto &it : arr) {
      test += it + '0';
    }

    assert(test == "124");

    // Reverse
    test = "";
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
      test += *it + '0';
    }

    assert(test == "421");
  }

  // ------------------------------------------------------------------
  // Usage with Arena 
  // ------------------------------------------------------------------
  {
    Arena a(1024);
    SArray<int> arr(a, 2);

    assert(
      arr.size() == 2 &&
      arr.used() == 0 &&
      arr.empty() == true &&
      a.used() == (
        (sizeof(int) * 2) +
        (sizeof(bool) * 2)
      )
    );

    assert(
      *arr.push(1) == 1 && *arr[0] == 1 &&
      *arr.push(2) == 2 && *arr[1] == 2 &&
      arr.used() == 2
    );

    arr.resize(5);

    assert(
      arr.size() == 5 &&
      *arr.push(3) == 3 && *arr[2] == 3 &&
      *arr.push(4) == 4 && *arr[3] == 4 &&
      *arr.push(5) == 5 && *arr[4] == 5 &&
      arr.push(6) == nullptr &&
      *arr[0] == 1 && *arr[1] == 2 &&
      arr.used() == 5 
    );

    arr.reset();

    assert(
      arr.used() == 0 &&
      arr[0] == nullptr &&
      arr[1] == nullptr &&
      arr[2] == nullptr &&
      arr[3] == nullptr &&
      arr[4] == nullptr
    );
  }

  // ------------------------------------------------------------------
  // Usage with a struct type 
  // ------------------------------------------------------------------
  {
    struct Person {
      char name[50];
      int age;
    };

    SArray<Person> arr(3);
    Person z{"John", 20};

    assert(
      (*arr.push(z)).age == 20 &&
      arr.used() == 1 &&
      arr.size() == 3 &&
      (*arr[0]).age == 20 &&
      arr[1] == nullptr
    );

    assert(
      (*arr.push_new(Person{"Doe", 30})).age == 30 &&
      arr.used() == 2 &&
      (*arr[1]).age == 30
    );
  }

  // ------------------------------------------------------------------
  // Usage with a class type 
  // ------------------------------------------------------------------
  {
    class Person {
    public:
      std::string name;
      int age;

      Person(const char* n, int &&a): name(n), age(a) { }
    };

    SArray<Person> arr(3);

    arr.push_new("John", 2);
    arr.push_new("Jane", 3);
    arr.push_new("James", 4);

    arr.erase(1);

    arr.resize(2);

    assert(
      arr.used() == 2 &&
      arr[0]->name == "John" &&
      arr[1]->name == "James"
    );

    *arr[1] = Person("Frank", 9);

    assert(
      arr.at(1)->name == "Frank" &&
      arr.at(1)->age == 9 
    );

    arr.replace_new(1, "Tom", 60);

    assert(
      arr[1]->name == "Tom" &&
      arr[1]->age == 60
    );
  }

  // ------------------------------------------------------------------
  // SArrayFixed usage
  // ------------------------------------------------------------------
  {
    SArrayFixed<int, 10> fixed = {1,2,3};

    assert(
      fixed.size() == 10 &&
      fixed.used() == 3 &&
      *fixed[0] == 1 &&
      *fixed[1] == 2 && 
      *fixed[2] == 3 
    );

    fixed = {4,5,6};

    assert(
      fixed.used() == 3 &&
      *fixed[0] == 4 &&
      *fixed[1] == 5 && 
      *fixed[2] == 6 
    );
  }

  // ------------------------------------------------------------------
  // SArrayFixed and SArray cross assignments 
  // ------------------------------------------------------------------
  {
    SArray<int> arr(30, {1,2,3});
    SArrayFixed<int, 30> fixed = {4,5,6};

    arr = fixed;

    assert(
      *arr[0] == 4 && 
      *arr[1] == 5 && 
      *arr[2] == 6 
    );

    arr = {1,2,3};

    fixed = arr;

    assert(
      *fixed[0] == 1 && 
      *fixed[1] == 2 && 
      *fixed[2] == 3 
    );

    SArrayFixed<int, 10> fixed2(arr);

    assert(
      *fixed2[0] == 1 && 
      *fixed2[1] == 2 && 
      *fixed2[2] == 3 
    );

    SArray<int> arr2(10, fixed);

    assert(
      *arr2[0] == 1 && 
      *arr2[1] == 2 && 
      *arr2[2] == 3 
    );

    Arena arena(100);
    SArray<int> arena_arr(arena, 3, fixed);

    assert(
      *arena_arr[0] == 1 && 
      *arena_arr[1] == 2 && 
      *arena_arr[2] == 3 
    );
  }

  // ------------------------------------------------------------------
  // Assignments from std::vector 
  // ------------------------------------------------------------------
  {
    std::vector<int> vec = {1,2,3};

    SArrayFixed<int, 10> arr(vec);

    assert(
      arr.used() == 3 &&
      *arr[0] == 1 &&
      *arr[1] == 2 &&
      *arr[2] == 3
    );

    vec = {4,5,6};

    arr = vec;

    assert(
      arr.used() == 3 &&
      *arr[0] == 4 &&
      *arr[1] == 5 &&
      *arr[2] == 6 
    );
  }

  // ------------------------------------------------------------------
  // Insert usage 
  // ------------------------------------------------------------------
  {
    // Insert int
    {
      SArray<int> arr(10, {1,3,4});

      // Insert 2, two times, at position 1
      arr.insert(++arr.begin(), 2, 2); 

      assert(
        arr.used() == 5 &&
        *arr[0] == 1 &&
        *arr[1] == 2 &&
        *arr[2] == 2 &&
        *arr[3] == 3 &&
        *arr[4] == 4
      );

      // Insert 0 at position 0 
      arr.insert(arr.begin(), 0);

      assert(
        arr.used() == 6 &&
        *arr[0] == 0 &&
        *arr[1] == 1 &&
        *arr[2] == 2 &&
        *arr[3] == 2 &&
        *arr[4] == 3 &&
        *arr[5] == 4
      );

      // Insert 5 at position 6 
      arr.insert(arr.begin() + 6, 5);

      assert(
        arr.used() == 7 &&
        *arr[0] == 0 &&
        *arr[1] == 1 &&
        *arr[2] == 2 &&
        *arr[3] == 2 &&
        *arr[4] == 3 &&
        *arr[5] == 4 &&
        *arr[6] == 5 
      );
    }

    // Insert std::string
    {
      SArray<std::string> arr(10, {"first", "third", "fourth"});
      arr.insert(++arr.begin(), 4, "second 4 times!"); 

      assert(
        arr.used() == 7 &&
        *arr[0] == "first" &&
        *arr[1] == "second 4 times!" &&
        *arr[2] == "second 4 times!" &&
        *arr[3] == "second 4 times!" &&
        *arr[4] == "second 4 times!" &&
        *arr[5] == "third" &&
        *arr[6] == "fourth"
      );
    }

    // Insert initializer list
    {
      SArray<std::string> arr(10, {"1", "5", "6"});

      arr.insert(++arr.begin(), {"2", "3", "4"});

      assert(
        arr.used() == 6 &&
        *arr[0] == "1" &&
        *arr[1] == "2" &&
        *arr[2] == "3" &&
        *arr[3] == "4" &&
        *arr[4] == "5" &&
        *arr[5] == "6"
      );
    }

    // insert_new 
    {
      struct Foo {
        int bar;
        std::string foo;

        Foo(int _bar, std::string _foo) : bar(_bar), foo(_foo) {}
      };

      SArray<Foo> arr(10);

      arr.insert_new(0, 2, "Second");
      arr.insert_new(0, 1, "First");

      assert(
        arr.used() == 2 &&
        arr[0]->bar == 1 &&
        arr[0]->foo == "First" &&
        arr[1]->bar == 2 &&
        arr[1]->foo == "Second"
      );

      arr.insert_new(2, 3, "Third");

      assert(
        arr.used() == 3 &&
        arr[0]->bar == 1 &&
        arr[0]->foo == "First" &&
        arr[1]->bar == 2 &&
        arr[1]->foo == "Second" &&
        arr[2]->bar == 3 &&
        arr[2]->foo == "Third"
      );

      arr.insert_new(arr.begin() + 1, 02, "Before second!");

      assert(
        arr.used() == 4 &&
        arr[1]->bar == 02 &&
        arr[1]->foo == "Before second!"
      );
    }
  }

  // ------------------------------------------------------------------
  // Usage with std::unique_ptr 
  // ------------------------------------------------------------------
  {
    SArray<std::unique_ptr<char>> arr(10);

    char* str = new char[20];
    strcpy(str, "Hello world!!");

    arr.push(str);

    assert(strcmp(arr[0]->get(), "Hello world!!") == 0);
  }
}
