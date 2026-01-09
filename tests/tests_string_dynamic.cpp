// COMPILE: g++ -std=c++11 -g -Wall -fsanitize=address tests_string_dynamic.cpp

#include "../src/string.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Running String tests...\n";

  // ------------------------------------------------------------------
  // Basic usage
  // ------------------------------------------------------------------
  {
    apc::str str = "Test!";
    apc::str str2 = apc::str("Test2!");

    assert(
      str == "Test!" &&
      str != "Test" &&
      strcmp(str.c_str(), "Test!") == 0 &&
      str.size() == 32 &&
      str.used() == 6
    );

    assert(
      str2 == "Test2!" &&
      str2 != "Test" &&
      strcmp(str2.c_str(), "Test2!") == 0 &&
      str2.size() == 32 &&
      str2.used() == 7 
    );

    assert(
      str.substr(0) == "Test!" &&
      str.substr(0, 1) == "T" &&
      str.substr(2, 3) == "st!"
    );
  }

  // ------------------------------------------------------------------
  // Append 
  // ------------------------------------------------------------------
  {
    apc::str str;

    str += "1";
    str += apc::str16("2");
    str.append(apc::str16("3"));
    str.append("4");

    assert(
      str == "1234" &&
      str.size() == 32 &&
      str.used() == 5 
    );

    str += str;

    assert(
      str == "12341234" &&
      str.used() == 9 
    );

    apc::str str2;
    str2 += str2;
    
    assert(str2.used() == 0 && str2 == "");
  }

  // ------------------------------------------------------------------
  // Dynamically grow beyond the static size 
  // ------------------------------------------------------------------
  {
    apc::str str = "1234";

    assert(
      str == "1234" &&
      str.size() == 32 &&
      str.used() == 5 
    );

    for(int i = 0; i < 3; i++) {
      str += "[a sentence...]"; // 15 chars
    }

    assert(
      str.size() == 32 * 2 && // size doubled
      str.used() == 50 // 4 + 15 + 15 + 15 + 1 (null char)
    );

    str += str;

    assert(
      str.size() == 32 * 4 && // size doubled
      str.used() == 99 // ((4 + 15 + 15 + 15) * 2) + 1 (null char)
    );
  }

  // ------------------------------------------------------------------
  // Bounds checking 
  // ------------------------------------------------------------------
  {
    apc::str str = "1234567891234567";

    assert(
      str == "1234567891234567"
    );

    str = str.substr(0, 5);
    str += "6789";

    assert(
      str == "123456789"
    );

    assert(
      str[0] == '1' &&
      str[8] == '9'
    );

    apc::str16 order;
    for(const auto it : str) {
      char tmp[2] = {it, '\0'};
      order += tmp;
    }

    assert(
      order == "123456789"
    );

    apc::str16 reverse;
    for(auto it = str.rbegin(); it != str.rend(); ++it) {
      char tmp[2] = {*it, '\0'};
      reverse += tmp;
    }

    assert(
      reverse == "987654321"
    );
  }

  // ------------------------------------------------------------------
  // Works with Str16/Str32/Str64/Str128/Str256 (static strings) 
  // ------------------------------------------------------------------
  {
    apc::str str;
    str += apc::str16("1");
    str += apc::str32("2");
    str += apc::str64("3");
    str += apc::str128("4");
    str += apc::str256("5");

    assert(
      str.size() == 32 &&
      str.used() == 6 &&
      str == "12345"
    );
  }
}
