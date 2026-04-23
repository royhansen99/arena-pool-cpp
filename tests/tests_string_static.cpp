// COMPILE: g++ -std=c++11 -g -Wall -fsanitize=address tests_string_static.cpp

#include "../src/string.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Running String tests...\n";

  // ------------------------------------------------------------------
  // Basic usage
  // ------------------------------------------------------------------
  {
    apc::str16 str = "Test!";
    apc::str16 str2 = apc::str16("Test2!");

    assert(
      str == "Test!" &&
      str != "Test" &&
      str[0] == 'T' &&
      str.at(0) == 'T' &&
      str[4] == '!' &&
      str.at(4) == '!' &&
      strcmp(str.c_str(), "Test!") == 0 &&
      str.size() == 16 &&
      str.used() == 5 
    );

    assert(
      str2 == "Test2!" &&
      str2 != "Test" &&
      strcmp(str2.c_str(), "Test2!") == 0 &&
      str2.size() == 16 &&
      str2.used() == 6 
    );

    assert(
      str.substr(0) == "Test!" &&
      str.substr(0, 1) == "T" &&
      str.substr(2, 3) == "st!"
    );

    str[0] = 't';
    str.at(4) = '_';

    assert(str == "test_");
  }

  // ------------------------------------------------------------------
  // Append 
  // ------------------------------------------------------------------
  {
    apc::str32 str;

    str += "1";
    str += apc::str16("2");
    str.append(apc::str16("3"));
    str.append("4");

    assert(
      str == "1234" &&
      str.size() == 32 &&
      str.used() == 4 
    );

    str += str;

    assert(
      str == "12341234" &&
      str.used() == 8 
    );

    apc::str32 str2;
    str2 += str2;
    
    assert(str2.used() == 0 && str2 == "");
  }

  // ------------------------------------------------------------------
  // Bounds checking 
  // ------------------------------------------------------------------
  {
    apc::str16 str = "12345678912345678";

    assert(
      str == "1234567891234567"
    );

    // Won't be appended since string is already full.
    str += "way out of bound";

    assert(
      str == "1234567891234567" &&
      str.size() == 16 &&
      str.used() == 16
    );

    str = str.substr(0, 5);

    assert(
      str == "12345"
    );

    // Last part truncated, since not enough space for everything.
    str += "|678912345678";

    assert(
      str == "12345|6789123456" &&
      str.size() == 16 &&
      str.used() == 16
    );

    assert(
      str[0] == '1' &&
      str[14] == '5'
    );

    apc::str16 order;
    for(const auto it : str) {
      char tmp[2] = {it, '\0'};
      order += tmp;
    }

    assert(
      order == "12345|6789123456"
    );

    apc::str16 reverse;
    for(auto it = str.rbegin(); it != str.rend(); ++it) {
      char tmp[2] = {*it, '\0'};
      reverse += tmp;
    }

    assert(
      reverse == "6543219876|54321"
    );
  }

  // ------------------------------------------------------------------
  // Works with Str (dynamic string) 
  // ------------------------------------------------------------------
  {
    apc::str16 str16 = apc::str("hello!");
    apc::str32 str32 = apc::str("hello!");
    apc::str64 str64 = apc::str("hello!");
    apc::str128 str128 = apc::str("hello!");
    apc::str256 str256 = apc::str("hello!");

    assert(
      str16 == "hello!" &&
      str32 == "hello!" &&
      str64 == "hello!" &&
      str128 == "hello!" &&
      str256 == "hello!"
    );
  }
}
