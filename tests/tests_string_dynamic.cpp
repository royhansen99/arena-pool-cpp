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
      str.used() == 5 
    );

    assert(
      str2 == "Test2!" &&
      str2 != "Test" &&
      strcmp(str2.c_str(), "Test2!") == 0 &&
      str2.size() == 32 &&
      str2.used() == 6 
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
      str.used() == 4 
    );

    str += str;

    assert(
      str == "12341234" &&
      str.used() == 8 
    );

    apc::str str2;
    str2 += str2;
    
    assert(str2.used() == 0 && str2 == "");
  }

  // ------------------------------------------------------------------
  // Dynamically grow beyond the static size 
  // ------------------------------------------------------------------
  {
    {
      apc::str str = "1234";

      assert(
        str == "1234" &&
        str.size() == 32 &&
        str.used() == 4 
      );

      for(int i = 0; i < 3; i++) {
        str += "[a sentence...]"; // 15 chars
      }

      assert(
        str.size() == 32 * 2 && // size doubled
        str.used() == 49 // 4 + 15 + 15 + 15
      );

      str += str;

      assert(
        str.size() == 32 * 4 && // size doubled
        str.used() == 98 // ((4 + 15 + 15 + 15) * 2)
      );
    }

    {
      apc::str_dynamic<11> str = "Hello world";

      // Fits exactly, so size remains the initial 11 chars.
      assert(
        str == "Hello world" &&
        str.used() == 11 &&
        str.size() == 11
      );

      str = "Hello world!";

      // Exceeds initial size by 1 chars, so size is doubled from
      // the initial.
      assert(
        str == "Hello world!" &&
        str.used() == 12 &&
        str.size() == 22 
      );
    }
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
      str.used() == 5 &&
      str == "12345"
    );
  }

  // ------------------------------------------------------------------
  // Erase 
  // (FYI: The null character `\0` is also counted in `used()`,
  // unless the string is empty - in that case `used() == 0`) 
  // ------------------------------------------------------------------
  {
    apc::str s = "Hello world!";

    {
      apc::str s0, s1, s2, s3, s4, s5, s6, s7, s8, s9;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = s9 = s;

      assert(
        s.used() == 12 &&
        s0.erase(0) == "" && s0.used() == 0 &&
        s1.erase(5) == "Hello" && s1.used() == 5 &&
        s2.erase(5, 1) == "Helloworld!" && s2.used() == 11 && 
        s3.erase(5, 6) == "Hello!" && s3.used() == 6 && 
        s4.erase(5, 7) == "Hello" && s4.used() == 5 && 
        s5.erase(1) == "H" && s5.used() == 1 && 
        s6.erase(11) == "Hello world" && s6.used() == 11 && 
        s7.erase(12) == "Hello world!" && s7.used() == 12 && 
        s8.erase(0, 12) == "" && s8.used() == 0 && 
        s9.erase(0, 11) == "!" && s9.used() == 1 
      );
    }

    // Erase with iterators()
    {
      apc::str s0, s1, s2, s3, s4, s5, s6;
      s0 = s1 = s2 = s3 = s4 = s5 = s6 = s;

      assert(
        s.used() == 12 &&
        s0.erase(s0.begin(), s0.end()) == "" && s0.used() == 0 &&
        s1.erase(s1.begin(), s1.begin() + 1) == "ello world!" && s1.used() == 11 && 
        s2.erase(s2.begin(), s2.end() - 3) == "ld!" && s2.used() == 3 && 
        s3.erase(s3.begin() + 1, s3.end() - 3) == "Hld!" && s3.used() == 4 && 
        s4.erase(s4.begin()) == "" && s4.used() == 0 &&
        s5.erase(s5.begin() + 1) == "H" && s5.used() == 1 &&
        s6.erase(s6.end() - 7) == "Hello" && s6.used() == 5 
      );
    }
  }

  // ------------------------------------------------------------------
  // Replace
  // ------------------------------------------------------------------
  {
    apc::str s = "Hello AAworld!BB";

    apc::str s0, s1, s2, s3, s4, s5, s6, s7, s8, s9;
    s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = s9 = s;

    assert(
      s0.replace(14, 2, "XXX", 0) == "Hello AAworld!XXX" && s0.used() == 17 &&
      s1.replace(6, 1, "XXX", 0) == "Hello XXXAworld!BB" && s1.used() == 18 && 
      s2.replace(5, apc::str::npos, "XXX", 0) == "HelloXXX" && s2.used() == 8 && 
      s3.replace(5, apc::str::npos, "123", 1, 1) == "Hello2" && s3.used() == 6 && 
      s4.replace(5, apc::str::npos, "123") == "Hello123" && s4.used() == 8 &&
      s5.replace(5, apc::str::npos, apc::str32("123"))  == "Hello123" && s5.used() == 8 &&
      s6.replace(5, apc::str::npos, apc::str("123"))  == "Hello123" && s6.used() == 8 
    );
  }

  // ------------------------------------------------------------------
  // Find 
  // ------------------------------------------------------------------
  {
    apc::str s = "Hello world!";

    assert(
      s.find("H") == 0 &&
      s.find("H", 1) == apc::str::npos &&
      s.find("world") == 6 &&
      s.find("l", 5) == 9 &&
      s.find(apc::str8("world")) == 6 &&
      s.find(apc::str("world")) == 6 &&
      s.rfind("l") == 9 &&
      s.rfind("o") == 7 && 
      s.rfind("123world", 3) == 6 && 
      s.rfind("Hello") == 0 && 
      s.rfind(apc::str("Hello")) == 0 && 
      s.rfind(apc::str16("Hello")) == 0 
    );
  }

  // ------------------------------------------------------------------
  // Shrink 
  // ------------------------------------------------------------------
  {
    apc::str s = "Hello world!";
    s.resize(100);

    assert(
      s.size() == 100 &&
      s.used() == 12 
    );

    s.shrink_to_fit(); // Will shrink to 32 which is the
                       // default minimum size.

    assert(
      s.size() == 32 &&
      s.used() == 12 &&
      s == "Hello world!"
    );
  }

  // ------------------------------------------------------------------
  // Trim 
  // ------------------------------------------------------------------
  {
    {
      apc::str s = "\r\n\t    Hello world!!!   \r\n   ";

      assert(s.used() == 29); 

      s.trim();

      assert(s.used() == 14); 
      assert(s == "Hello world!!!");
    }

    {
      apc::str s = "   \r\n\t    \r\n";

      assert(s.used() == 12);

      s.trim();

      assert(s.used() == 0);
      assert(s == "");
    }
  }
}
