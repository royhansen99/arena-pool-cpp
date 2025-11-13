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

    arr.reset();

    assert(
      arr.used() == 0
    );
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
  }
}
