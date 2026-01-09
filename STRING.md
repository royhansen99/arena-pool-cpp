# strings-cpp

An efficient, size-optimized string implementation, featuring a  
hybrid dynamic/static allocation strategy.  
It offers both fixed-size string types and a dynamically growing  
type utilizing Small String Optimization (SSO).

`apc::str` is the dynamic string class, which has a static size of  
32 chars, but will move to heap allocation once you go past this  
size. Size will double each time the limit is reached.

And these are the fixed-size string classes:  
`apc::str16`, `apc::str32`, `apc::str64`, `apc::str128`, `apc::str256`  
These are not dynamic and will safely discard data that  
goes past the limit.

### Usage

Simply drop the single header string.h file directly in to your project.

```cpp
#include "./string.h"
#include <iostream>

int main() {
  apc::str32 str = "Hello";
  str += apc::str16(" world");
  str += apc::str64("!");

  std::cout << str << "\n"; // "Hello world!"

  str += str;

  std::cout << str << "\n"; // "Hello world!Hello world!"

  // "Used: 25 / 32"
  std::cout << "Used: " << str.used() << " / " << str.size() << "\n";

  // Will go beyond the fixed 32-char size,
  // so will discard the remainder that can't
  // fit.
  str += str;

  std::cout << str << "\n"; // "Hello world!Hello world!Hello w""

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
```
