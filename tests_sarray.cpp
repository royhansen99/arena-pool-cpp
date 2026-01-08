// COMPILE: g++ -std=c++11 -Wall -fsanitize=address tests_sarray.cpp

#include "./Arena.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Running apc::vector tests...\n";

  // ------------------------------------------------------------------
  // Basic usage (malloc) 
  // ------------------------------------------------------------------
  {
    apc::vector<int> arr(4);

    assert(
      arr.size() == 4 &&
      arr.used() == 0 &&
      arr.empty() == true
    );

    arr = {1,2};

    assert(
      arr.used() == 2 &&
      arr[0] == 1 &&
      arr[1] == 2
    );

    arr = {};

    auto* p1 = arr.push_new(1);
    auto* p2 = arr.push_new(2);
    int three = 3;
    auto* p3 = arr.push(three);
    int four = 4;
    auto* p4 = arr.push(four);

    assert(
      arr[0] == 1 && *p1 == 1 && 
      arr[1] == 2 && *p2 == 2 && 
      arr[2] == 3 && *p3 == 3 && 
      arr[3] == 4 && *p4 == 4 && 
      arr.at(0) == 1 &&
      arr.at(1) == 2 && 
      arr.at(2) == 3 && 
      arr.at(3) == 4 && 
      arr.used() == 4 &&
      arr.size() == 4 &&
      arr.empty() == false &&
      *arr.first() == 1 && *arr.last() == 4
    );

    arr.pop();

    assert(arr.used() == 3);

    arr.erase(0);

    assert(arr.used() == 2);

    arr.erase(1);

    assert(
      arr[0] == 2 &&
      *arr.push(3) == 3 &&
      *arr.push(7) == 7 &&
      arr.used() == 3
    );

    arr.resize(6);

    assert(
      arr.size() == 6 &&
      *arr.push(10) == 10 &&
      *arr.push(11) == 11 &&
      *arr.push(12) == 12 &&
      arr[0] == 2 &&
      arr[1] == 3 &&
      arr[2] == 7
    );

    int i = 0;
    for(auto &it : arr) {
      assert(
        (i == 0 && it == 2) || 
        (i == 1 && it == 3) || 
        (i == 2 && it == 7) || 
        (i == 3 && it == 10) || 
        (i == 4 && it == 11) || 
        (i == 5 && it == 12) 
      );

      i++;
    }

    arr.erase_ptr(&arr[2]);

    assert(arr.used() == 5);

    arr.resize(3);

    assert(
      arr.used() == 3 &&
      arr.size() == 3 &&
      arr[0] == 2 &&
      arr[1] == 3 &&
      arr[2] == 10
    );

    arr.replace(1, 900);

    assert(arr[1] == 900);

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
    apc::vector<int> arr(3, {1,2,3});
    
    assert(
      arr.size() == 3 &&
      arr.used() == 3 &&
      arr[0] == 1 &&
      arr[1] == 2 &&
      arr[2] == 3
    );

    apc::arena arena(32);
    apc::vector<int> arena_arr(arena, 3, {1,2,3});

    assert(
      arena_arr.size() == 3 &&
      arena_arr.used() == 3 &&
      arena_arr[0] == 1 &&
      arena_arr[1] == 2 &&
      arena_arr[2] == 3
    );

    apc::vector_fixed<int, 3> fixed({1,2,3});

    assert(
      fixed.size() == 3 &&
      fixed.used() == 3 &&
      fixed[0] == 1 &&
      fixed[1] == 2 && 
      fixed[2] == 3 
    );
  }

  // ------------------------------------------------------------------
  // Iteration 
  // ------------------------------------------------------------------
  {
    apc::vector<int> arr(6);
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
    // 12345
    // 1235
    //

    // Reverse
    test = "";
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
      test += *it + '0';
    }

    assert(test == "543210");

    arr.erase(0); // Erase 0
    arr.erase(3); // Erase 4
    arr.erase(5); // Out of bound, does nothing.

    test = "";
    for(const auto &it : arr) {
      test += it + '0';
    }

    assert(test == "1235");

    // Reverse
    test = "";
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
      test += *it + '0';
    }

    assert(test == "5321");
  }

  // ------------------------------------------------------------------
  // Usage with Arena 
  // ------------------------------------------------------------------
  {
    apc::arena a(1024);
    apc::vector<int> arr(a, 2);

    assert(
      arr.size() == 2 &&
      arr.used() == 0 &&
      arr.empty() == true &&
      a.used() == sizeof(int) * 2
    );

    assert(
      *arr.push(1) == 1 && arr[0] == 1 &&
      *arr.push(2) == 2 && arr[1] == 2 &&
      arr.used() == 2
    );

    arr.resize(5);

    assert(
      arr.size() == 5 &&
      *arr.push(3) == 3 && arr[2] == 3 &&
      *arr.push(4) == 4 && arr[3] == 4 &&
      *arr.push(5) == 5 && arr[4] == 5 &&
      arr[0] == 1 && arr[1] == 2 &&
      arr.used() == 5 
    );

    arr.reset();

    assert(arr.used() == 0);
  }

  // ------------------------------------------------------------------
  // Usage with a struct type 
  // ------------------------------------------------------------------
  {
    struct Person {
      char name[50];
      int age;
    };

    apc::vector<Person> arr(3);
    Person z{"John", 20};

    assert(
      (*arr.push(z)).age == 20 &&
      arr.used() == 1 &&
      arr.size() == 3 &&
      arr[0].age == 20
    );

    assert(
      (*arr.push_new(Person{"Doe", 30})).age == 30 &&
      arr.used() == 2 &&
      arr[1].age == 30
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

    apc::vector<Person> arr(3);

    arr.push_new("John", 2);
    arr.push_new("Jane", 3);
    arr.push_new("James", 4);

    arr.erase(1);

    arr.resize(2);

    assert(
      arr.used() == 2 &&
      arr[0].name == "John" &&
      arr[1].name == "James"
    );

    arr[1] = Person("Frank", 9);

    assert(
      arr.at(1).name == "Frank" &&
      arr.at(1).age == 9 
    );

    arr.replace_new(1, "Tom", 60);

    assert(
      arr[1].name == "Tom" &&
      arr[1].age == 60
    );
  }

  // ------------------------------------------------------------------
  // apc::vector_fixed usage
  // ------------------------------------------------------------------
  {
    apc::vector_fixed<int, 10> fixed = {1,2,3};

    assert(
      fixed.size() == 10 &&
      fixed.used() == 3 &&
      fixed[0] == 1 &&
      fixed[1] == 2 && 
      fixed[2] == 3 
    );

    fixed = {4,5,6};

    assert(
      fixed.used() == 3 &&
      fixed[0] == 4 &&
      fixed[1] == 5 && 
      fixed[2] == 6 
    );

    // Out of bounds
    apc::vector_fixed<int, 2> f = {1,2};
    assert(f.push(3) == nullptr);
  }

  // ------------------------------------------------------------------
  // apc::vector_fixed and apc::vector cross assignments 
  // ------------------------------------------------------------------
  {
    apc::vector<int> arr(30, {1,2,3});
    apc::vector_fixed<int, 30> fixed = {4,5,6};

    arr = fixed;

    assert(
      arr[0] == 4 && 
      arr[1] == 5 && 
      arr[2] == 6 
    );

    arr = {1,2,3};

    fixed = arr;

    assert(
      fixed[0] == 1 && 
      fixed[1] == 2 && 
      fixed[2] == 3 
    );

    apc::vector_fixed<int, 10> fixed2(arr);

    assert(
      fixed2[0] == 1 && 
      fixed2[1] == 2 && 
      fixed2[2] == 3 
    );

    apc::vector<int> arr2(10, fixed);

    assert(
      arr2[0] == 1 && 
      arr2[1] == 2 && 
      arr2[2] == 3 
    );

    apc::arena arena(100);
    apc::vector<int> arena_arr(arena, 3, fixed);

    assert(
      arena_arr[0] == 1 && 
      arena_arr[1] == 2 && 
      arena_arr[2] == 3 
    );
  }

  // ------------------------------------------------------------------
  // Assignments from std::vector 
  // ------------------------------------------------------------------
  {
    std::vector<int> vec = {1,2,3};

    apc::vector_fixed<int, 10> arr(vec);

    assert(
      arr.used() == 3 &&
      arr[0] == 1 &&
      arr[1] == 2 &&
      arr[2] == 3
    );

    vec = {4,5,6};

    arr = vec;

    assert(
      arr.used() == 3 &&
      arr[0] == 4 &&
      arr[1] == 5 &&
      arr[2] == 6 
    );
  }

  // ------------------------------------------------------------------
  // Insert usage 
  // ------------------------------------------------------------------
  {
    // Insert int
    {
      apc::vector<int> arr(10, {1,3,4});

      // Insert 2, two times, at position 1
      arr.insert(++arr.begin(), 2, 2); 

      assert(
        arr.used() == 5 &&
        arr[0] == 1 &&
        arr[1] == 2 &&
        arr[2] == 2 &&
        arr[3] == 3 &&
        arr[4] == 4
      );

      // Insert 0 at position 0 
      arr.insert(arr.begin(), 0);

      assert(
        arr.used() == 6 &&
        arr[0] == 0 &&
        arr[1] == 1 &&
        arr[2] == 2 &&
        arr[3] == 2 &&
        arr[4] == 3 &&
        arr[5] == 4
      );

      // Insert 5 at position 6 
      arr.insert(arr.begin() + 6, 5);

      assert(
        arr.used() == 7 &&
        arr[0] == 0 &&
        arr[1] == 1 &&
        arr[2] == 2 &&
        arr[3] == 2 &&
        arr[4] == 3 &&
        arr[5] == 4 &&
        arr[6] == 5 
      );
    }

    // Insert std::string
    {
      apc::vector<std::string> arr(10, {"first", "third", "fourth"});
      arr.insert(++arr.begin(), 4, "second 4 times!"); 

      assert(
        arr.used() == 7 &&
        arr[0] == "first" &&
        arr[1] == "second 4 times!" &&
        arr[2] == "second 4 times!" &&
        arr[3] == "second 4 times!" &&
        arr[4] == "second 4 times!" &&
        arr[5] == "third" &&
        arr[6] == "fourth"
      );
    }

    // Insert initializer list
    {
      apc::vector<std::string> arr(10, {"1", "5", "6"});

      arr.insert(++arr.begin(), {"2", "3", "4"});

      assert(
        arr.used() == 6 &&
        arr[0] == "1" &&
        arr[1] == "2" &&
        arr[2] == "3" &&
        arr[3] == "4" &&
        arr[4] == "5" &&
        arr[5] == "6"
      );
    }

    // insert_new 
    {
      struct Foo {
        int bar;
        std::string foo;

        Foo(int _bar, std::string _foo) : bar(_bar), foo(_foo) {}
      };

      apc::vector<Foo> arr(10);

      arr.insert_new(0, 2, "Second");
      arr.insert_new(0, 1, "First");

      assert(
        arr.used() == 2 &&
        arr[0].bar == 1 &&
        arr[0].foo == "First" &&
        arr[1].bar == 2 &&
        arr[1].foo == "Second"
      );

      arr.insert_new(2, 3, "Third");

      assert(
        arr.used() == 3 &&
        arr[0].bar == 1 &&
        arr[0].foo == "First" &&
        arr[1].bar == 2 &&
        arr[1].foo == "Second" &&
        arr[2].bar == 3 &&
        arr[2].foo == "Third"
      );

      arr.insert_new(arr.begin() + 1, 02, "Before second!");

      assert(
        arr.used() == 4 &&
        arr[1].bar == 02 &&
        arr[1].foo == "Before second!"
      );
    }
  }

  // ------------------------------------------------------------------
  // Usage with std::unique_ptr 
  // ------------------------------------------------------------------
  {
    apc::vector<std::unique_ptr<char>> arr(10);

    char* str = new char[20];
    strcpy(str, "Hello world!!");

    arr.push(str);

    assert(strcmp(arr[0].get(), "Hello world!!") == 0);
  }

  // ------------------------------------------------------------------
  // Copy constructor 
  // ------------------------------------------------------------------
  {
    apc::arena arena(300);
    apc::vector<int> org(arena, 10, {1,2,3,4,5,6});

    apc::vector<int> copy(org);
    std::string copy_str;

    for(const auto &i : copy)
      copy_str += std::to_string(i); 

    assert(copy_str == "123456");

    assert(org.arena() == copy.arena());

    apc::vector_fixed<int, 3> fixed({100,200});

    apc::vector<int> copy2(fixed);
    
    assert(
      copy2.size() == 3 &&
      copy2.used() == 2 &&
      copy2[0] == 100 &&
      copy2[1] == 200
    );
  }

  // ------------------------------------------------------------------
  // Move constructor 
  // ------------------------------------------------------------------
  {
    apc::arena arena(100);
    apc::vector<int> org(arena, 5, {1,2,3,4,5});

    apc::vector<int> copy(std::move(org));

    std::string copy_str;
    for(const auto &i : copy)
      copy_str += std::to_string(i); 

    assert(
      org.used() == 0 &&
      org.size() == 0 &&
      copy.used() == 5 &&
      copy.size() == 5 &&
      copy_str == "12345"
    );
  }

  // ------------------------------------------------------------------
  // Automatically grow apc::vector 
  // ------------------------------------------------------------------
  {
    apc::vector<int> arr;
    
    assert(arr.size() == 0);

    arr.push(1);

    assert(arr.size() == 1);

    arr.push(2);

    assert(arr.size() == 2);

    arr.push(3);

    assert(
      arr.size() == 4 && // doubled from previous size
      arr.used() == 3 &&
      arr[0] == 1 && arr[1] == 2
    );
  }
}
