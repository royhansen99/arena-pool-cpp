// Compile: g++ -std=c++11 -g -Wall -fsanitize=address string_example.cpp

#include "../src/string.h"
#include <iostream>

int main() {
  apc::str32 str = "Hello";
  str += apc::str16(" world");
  str += apc::str64("!");

  std::cout << str << "\n"; // "Hello world!"

  str += str;

  std::cout << str << "\n"; // "Hello world!Hello world!"

  // "Used: 24 / 32"
  std::cout << "Used: " << str.used() << " / " << str.size() << "\n";

  // Will go beyond the fixed 32-char size,
  // so will discard the remainder that can't
  // fit.
  str += str;

  std::cout << str << "\n"; // "Hello world!Hello world!Hello wo"

  // "Used: 32 / 32"
  std::cout << "Used: " << str.used() << " / " << str.size() << "\n";

  str = str.substr(0, 5);

  std::cout << str << "\n"; // "Hello"

  str == "Hello"; // true
  str != "Hello world!Hello world!"; // true

  str[0] == 'H'; // true
  str.at(0) == 'H'; // true
  str[0] = 'h'; // Changed value of first position.
  str.at(0) = 'h'; // Changed value of first position.

  // Loop through one char at a time. 
  for(const char& it : str)
    std::cout << it << "\n";

  // Reverse loop.
  for(auto it = str.rbegin(); it != str.rend(); ++it)
    std::cout << *it << "\n";
}
