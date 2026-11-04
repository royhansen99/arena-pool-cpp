// COMPILE: g++ -std=c++11 -Wall -g -fsanitize=address tests/tests_treemap.cpp
#define APC_TREEMAP_TESTING
#include <cassert>
#include <string>
#include <cstdio>
#include "../src/arena.h"
#include "../src/treemap.h"

int main() {
  { /* Test insert structure, Left-Left and Right-Right. 
     * Insert: 10, 20, 30, 40, 50, 2
     *
     *          [20 Black]
     *         /          \
     *    [10 Black]   [40 Black]
     *    /            /        \
     * [2 Red]     [30 Red]    [50 Red]
    */

    apc::treemap_fixed<int, int, 10> map;

    for(int v : {10, 20, 30, 40, 50, 2})
      map.insert(v, v);

    auto* twenty = map.get_root();

    assert(
      twenty && twenty->current == 20 && !twenty->is_red &&
      twenty->parent == nullptr
    );

    {
        auto* ten = twenty->left;

        assert(
          ten && ten->key == 10 && !ten->is_red &&
          ten->parent == twenty && ten->right == nullptr
        );

        auto *two = ten->left;

        assert(
          two && two->key == 2 && two->is_red &&
          two->parent == ten && two->left == nullptr &&
          two->right == nullptr
        );
    }

    {
      auto* forty = twenty->right;

      assert(
        forty && forty->key == 40 && !forty->is_red &&
        forty->parent == twenty
      );

      {
        auto* thirty = forty->left;

        assert(
          thirty && thirty->key == 30 && thirty->is_red &&
          thirty->parent == forty && thirty->left == nullptr &&
          thirty->right == nullptr
        );
      }

      {
        auto* fifty = forty->right;

        assert(
          fifty && fifty->key == 50 && fifty->is_red &&
          fifty->parent == forty && fifty->left == nullptr &&
          fifty->right == nullptr
        );
      }
    }
  }

  { /* Test insert structure, Right-Left. 
     * Insert: 10, 30, 20
     *
     *          [20 Black]
     *         /          \
     *    [10 Red]     [30 Red]
     *
    */

    apc::treemap_fixed<int, int, 10> map;

    for(int v : {10, 30, 20})
      map.insert(v, v);

    auto* twenty = map.get_root();

    assert(
      twenty && twenty->key == 20 && !twenty->is_red &&
      twenty->parent == nullptr
    );

    {
      auto* ten = twenty->left;

      assert(
        ten && ten->key == 10 && ten->is_red &&
        ten->parent == twenty && ten->left == nullptr &&
        ten->right == nullptr
      );
    }

    {
      auto* thirty = twenty->right;

      assert(
        thirty && thirty->key == 30 && thirty->is_red &&
        thirty->parent == twenty && thirty->left == nullptr &&
        thirty->right == nullptr
      );
    }
  }

  { /* Test insert structure, Left-Right. 
     * Insert: 30, 10, 20.
     *
     *          [20 Black]
     *         /          \
     *    [10 Red]     [30 Red]
     *
    */

    apc::treemap_fixed<int, int, 10> map;

    for(int v : {30, 10, 20})
      map.insert(v, v);

    auto* twenty = map.get_root();

    assert(
      twenty && twenty->key == 20 && !twenty->is_red &&
      twenty->parent == nullptr
    );

    {
      auto* ten = twenty->left;

      assert(
        ten && ten->key == 10 && ten->is_red &&
        ten->parent == twenty && ten->left == nullptr &&
        ten->right == nullptr
      );
    }

    {
      auto* thirty = twenty->right;

      assert(
        thirty && thirty->key == 30 && thirty->is_red &&
        thirty->parent == twenty && thirty->left == nullptr &&
        thirty->right == nullptr
      );
    }
  }

  { /* Test insert_new() */
    struct test {
      int foo;
      const char* bar;

      test(int f, const char* b) : foo(f), bar(b) {}
    };

    apc::treemap<test, int> map;

    map.insert_new(1, 100, "Test.."); 

    auto* item = map.find(1);

    assert(map.used() == 1 && item != nullptr &&
      strcmp(item->bar, "Test..") == 0);
  }

  { /* Test find() */
    apc::treemap_fixed<int, int, 30> map;

    auto list = {30, 10, 20, 5, 28, 1, 2, 3};

    for(int v : list)
      map.insert(v, v);

    for(int v : list) {
      auto* find = map.find(v);
      assert(find != nullptr && *find == v);
    }
  }

  { /* Test erase()
     *
     * Insert: 10, 20, 30, 40, 50, 2
     *
     *          [20 Black]
     *         /          \
     *    [10 Black]   [40 Black]
     *    /            /        \
     * [2 Red]     [30 Red]    [50 Red]
     *
     * ***********
     * ***********
     *
     * After delete 40:
     *
     *          [20 Black]
     *         /          \
     *    [10 Black]   [50 Black]
     *    /                     \
     * [2 Red]               [30 Red] 
     *
    */

    apc::treemap_fixed<int, int, 10> map;

    for(int v : {10, 20, 30, 40, 50, 2})
      map.insert(v, v);

    {
      assert(map.find(40) != nullptr && map.used() == 6);

      map.erase(40);

      assert(map.find(40) == nullptr && map.used() == 5);
    }

    {
      auto* twenty = map.get_root();

      assert(
        twenty && twenty->current == 20 && !twenty->is_red &&
        twenty->parent == nullptr
      );

      {
          auto* ten = twenty->left;

          assert(
            ten && ten->key == 10 && !ten->is_red &&
            ten->parent == twenty && ten->right == nullptr
          );

          auto *two = ten->left;

          assert(
            two && two->key == 2 && two->is_red &&
            two->parent == ten && two->left == nullptr &&
            two->right == nullptr
          );
      }

      {
        auto* fifty = twenty->right;

        assert(
          fifty && fifty->key == 50 && !fifty->is_red &&
          fifty->parent == twenty
        );

        {
          auto* thirty = fifty->left;

          assert(
            thirty && thirty->key == 30 && thirty->is_red &&
            thirty->parent == fifty && thirty->left == nullptr &&
            thirty->right == nullptr
          );
        }
      }
    }
  }

  { /* Test for-loop iteration */
    apc::treemap_fixed<int, int, 30> map;

    for(
      int v : {10, 20, 30, 40, 50, 2, 1, 3, 4, 5, 6, 51, 52, 53, 54,
        55, 56, 0}
    ) map.insert(v, v);

    map.erase(50);

    { int key = 10; assert(map.erase(key)); }

    assert(map.erase(map.begin())); // Erase by iterator (key: 0)

    assert(!map.erase(map.end())); // end() is an iterator->nullptr,
                                   // so this should not do anything.

    char num[3] = {'\0'};

    {
      std::string result;

      for(auto& v : map) {
        snprintf(num, 3, "%d", v);

        if(result.size()) result += ",";

        result += num;
      }

      assert(result == "1,2,3,4,5,6,20,30,40,51,52,53,54,55,56"); 
    }

    {
      std::string result;

      for(auto it = map.rbegin(); it != map.rend(); ++it) {
        snprintf(num, 3, "%d", *it);

        if(result.size()) result += ",";

        result += num;
      }

      assert(result == "56,55,54,53,52,51,40,30,20,6,5,4,3,2,1");
    }

    assert(*(map.begin()) == 1);
    assert(*(map.begin() + 1) == 2);
    assert(*(map.begin() + 10) == 52);

    assert(*(map.rbegin()) == 56);
    assert(*(map.rbegin() + 1) == 55);
    assert(*(map.rbegin() + 5) == 51);

    map.reset();

    assert(map.used() == 0);
  }

  { /* Test for-loop iteration with std::string key */
    apc::treemap_fixed<std::string, std::string, 20> map;

    const char* letters[] = {"y", "z", "x", "d", "e", "f", "a", "b", "c", "g"};

    for(const char* v : letters)
      map.insert(v, v);

    {
      std::string result;

      for(auto& it : map) {
        if(result != "") result += ",";
        result += it;
      }

      assert(result == "a,b,c,d,e,f,g,x,y,z");
    }

    {
      std::string result;

      for(auto it = map.rbegin(); it != map.rend(); ++it) {
        if(result != "") result += ",";
        result += *it;
      }

      assert(result == "z,y,x,g,f,e,d,c,b,a");
    }

    { /* Delete one-by-one */
      for(auto* v : letters)
        map.erase(v);

      assert(map.used() == 0);
    }
  }

  { /* Test fixed full */
    apc::treemap_fixed<int, int, 3> map;

    assert(map.insert(0, 0) && map.insert(1, 1) && map.insert(2, 2) &&
      map.insert(3, 3) == nullptr && map.used() == 3 && map.size() == 3); 
  }

  { /* Test dynamic/malloc growth */
    apc::treemap<int, int> map;

    assert(map.size() == 0);

    map.insert(1, 1);

    assert(map.size() == 1);

    map.insert(2, 2);

    assert(map.size() == 3);

    map.insert(3, 3);
      
    assert(map.size() == 3);
      
    map.insert(4, 4);

    assert(map.size() == 9);
      
    map.insert(5, 5);
    map.insert(6, 6);
    map.insert(7, 7);
    map.insert(8, 8);
    map.insert(9, 9);
      
    assert(map.size() == 9);
  }

  { /* Test allocation using arena */
    apc::arena arena(500);

    assert(arena.used() == 0);

    apc::treemap<int, int> map(arena, 20);

    auto arena_used = arena.used();

    assert(map.size() == 20 && map.used() == 0 && arena_used > 0);

    for(auto i = 0; i < 20; i++)
      map.insert(i, i);

    assert(map.size() == 20 && map.used() == 20 &&
      arena.used() == arena_used);

    assert(*map.begin() == 0); 
    assert(*map.rbegin() == 19); 
    assert(*map.find(15) == 15); 
  }

  return 0;
}
