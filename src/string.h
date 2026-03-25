/*
 * Package: arena_pool_cpp
 * Version: 0.2.1
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast memory allocator with
 *              an arena/bump-allocator, a pool-allocator, and a
 *              vector-like array-allocator. Also includes a string
 *              implementation.
 *              Single header c++11 library.
 */
#pragma once

#include <ostream>
#include <cstring>

namespace apc {

#define STRING_COMMON_ITERATOR(A) \
  class iterator { \
    char* it; \
    bool _reverse; \
  public: \
    explicit iterator(char& begin, bool reverse = false): \
      it(&begin), _reverse(reverse) { } \
    \
    char& operator*() const { return *it; } \
    \
    bool is_equal(const iterator& other) const { return it != other.it; }; \
    \
    bool operator!=(const iterator& other) const { return is_equal(other); } \
    \
    iterator& operator+(const size_t &num) { \
      if(_reverse) it -= num; \
      else it += num; \
      \
      return *this; \
    } \
    \
    iterator& operator++() { \
      return operator+(1); \
    } \
    \
    iterator& operator-(const size_t &num) { \
      if(_reverse) it += num; \
      else it -= num; \
      \
      return *this; \
    } \
  }; \
  \
  iterator begin() { \
    return iterator(buffer[0]); \
  } \
  \
  iterator rbegin() { \
    return iterator(buffer[_used - 2], true); \
  } \
  \
  iterator end() { \
    return iterator(buffer[_used - 1]); \
  } \
  \
  const iterator end() const { \
    return iterator(buffer[_used - 1]); \
  } \
  \
  iterator rend() { \
    char* ptr = buffer; \
    return iterator(*(ptr - 1)); \
  } \
  \
  const iterator rend() const { \
    char* ptr = buffer; \
    return iterator(*(ptr - 1)); \
  }

#define STRING_COMMON_METHODS(A) \
  template <size_t S> \
  A& insert(const size_t pos, const str_fixed<S>& other, const size_t sub_pos, const size_t len) { \
    return insert(pos, other.c_str(), sub_pos, len); \
  } \
  \
  template <size_t S> \
  A& insert(const size_t pos, const str_dynamic<S>& other, const size_t sub_pos, const size_t len) { \
    return insert(pos, other.c_str(), sub_pos, len); \
  } \
  \
  A& insert(const size_t pos, const char* other, const size_t len = npos) { \
    return insert(pos, other, 0, len); \
  } \
  \
  template <size_t S> \
  A& insert(const size_t pos, const str_fixed<S>& other, const size_t len = npos) { \
    return insert(pos, other.c_str(), 0, len); \
  } \
  \
  template <size_t S> \
  A& insert(const size_t pos, const str_dynamic<S>& other, const size_t len = npos) { \
    return insert(pos, other.c_str(), 0, len); \
  } \
  A& append(const char *other, const size_t len = npos) { \
    return insert(_used ? _used - 1 : _used, other, len); \
  } \
  \
  template <size_t S> \
  A& append(const str_fixed<S>& other, const size_t len = npos) { \
    return append(other.c_str(), len); \
  } \
  \
  template <size_t S> \
  A& append(const str_dynamic<S>& other, const size_t len = npos) { \
    return append(other.c_str(), len); \
  } \
  \
  A& erase(const size_t pos, size_t len = npos) { \
    if(!len || !_used || pos > _used - 1) return *this; \
    const size_t max = _used - 1 - pos; \
    if(max < len) len = max; \
    const size_t remaining = _used - 1 - (pos + len); \
    if(remaining) { \
      memmove(&buffer[pos], &buffer[pos + len], remaining); \
      buffer[pos + remaining] = '\0'; \
    } else { \
      buffer[pos] = '\0'; \
    } \
    _used = remaining || pos ? _used - len : 0; \
    return *this; \
  } \
  A& erase(const iterator &start, const iterator &end) { \
    if(_used && &*start < &buffer[_used - 1]) {\
      size_t pos = &*start - buffer; \
      size_t pos_end = (&*end - buffer); \
      erase(pos, pos_end - pos); \
    } \
    return *this; \
  } \
  \
  A& erase(const iterator &start) { \
    return erase(start, end()); \
  } \
  \
  A& replace(const size_t pos, size_t len, const char* s, const size_t subpos, const size_t sublen = npos) { \
    if(pos >= _used - 1) return *this; \
    erase(pos, len); \
    insert(pos, s, subpos, sublen); \
    return *this; \
  } \
  \
  A& replace(const size_t pos, size_t len, const char* s) { \
    return replace(pos, len, s, 0, npos); \
  } \
  \
  template <size_t S> \
  A& replace(const size_t pos, size_t len, const str_fixed<S> &other, const size_t subpos, const size_t sublen = npos) { \
    return replace(pos, len, other.c_str(), subpos, sublen); \
  } \
  \
  template <size_t S> \
  A& replace(const size_t pos, size_t len, const str_fixed<S> &other) { \
    return replace(pos, len, other.c_str(), 0, npos); \
  } \
  \
  template <size_t S> \
  A& replace(const size_t pos, size_t len, const str_dynamic<S> &other, const size_t subpos, const size_t sublen = npos) { \
    return replace(pos, len, other.c_str(), subpos, sublen); \
  } \
  \
  template <size_t S> \
  A& replace(const size_t pos, size_t len, const str_dynamic<S> &other) { \
    return replace(pos, len, other.c_str(), 0, npos); \
  } \
  \
  A& trim() { \
    if(!_used) return *this; \
    \
    size_t \
      size = _used - 1, \
      remaining = size, \
      trim_begin = 0, \
      trim_end = 0; \
    \
    for(size_t i = 0; i < size; i++) { \
      if(buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == ' ' || buffer[i] == '\t') trim_begin++; \
      else break; \
    } \
    \
    remaining -= trim_begin; \
    \
    if(!remaining) { \
      buffer[0] = '\0'; \
      _used = 0; \
      return *this; \
    } \
    \
    for(size_t i = size - 1; i >= 0; i--) { \
      if(buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == ' ' || buffer[i] == '\t') trim_end++; \
      else break; \
    } \
    \
    remaining -= trim_end; \
    \
    if(!remaining) { \
      buffer[0] = '\0'; \
      _used = 0; \
      return *this; \
    } \
    \
    if(trim_begin) { \
      memmove(buffer, &buffer[trim_begin], size - trim_begin); \
      buffer[size - trim_begin] = '\0'; \
    } \
    \
    if(trim_end) buffer[size - trim_begin - trim_end] = '\0'; \
    \
    _used = remaining + 1; \
    \
    return *this; \
  } \
  \
  size_t find(const char* other, size_t pos = 0) { \
    if(pos > _used - 1) return npos; \
    char* search = strstr(&buffer[pos], other); \
    return search ? search - buffer : npos; \
  } \
  \
  template <size_t s> \
  size_t find(const str_fixed<s>& other, size_t pos = 0) { \
    return find(other.c_str(), pos); \
  } \
  \
  template <size_t S> \
  size_t find(const str_dynamic<S>& other, size_t pos = 0) { \
    return find(other.c_str(), pos); \
  } \
  size_t rfind(const char* other, size_t other_pos = 0) { \
    const size_t length = strlen(other); \
    if(!length || other_pos > length - 1 || length > _used - 1) return npos; \
    size_t pos = _used - 1 - length; \
    while(pos >= 0) { \
      char* search = strstr(&buffer[pos], &other[other_pos]); \
      if(search) return search - buffer; \
      if(pos == 0) break; \
      \
      if(length > pos) pos = 0;\
      else pos -= length; \
    } \
    return npos; \
  } \
  \
  template <size_t S> \
  size_t rfind(const str_fixed<S>& other, size_t other_pos = 0) { \
    return rfind(other.c_str(), other_pos); \
  } \
  \
  template <size_t S> \
  size_t rfind(const str_dynamic<S>& other, size_t other_pos = 0) { \
    return rfind(other.c_str(), other_pos); \
  } \
  \
  int compare(const char *other) const { \
    return strcmp(buffer, other); \
  } \
  \
  template <size_t S> \
  int compare(const str_fixed<S>& other) const { \
    return compare(other.c_str()); \
  } \
  \
  template <size_t S> \
  int compare(const str_dynamic<S>& other) const { \
    return compare(other.c_str()); \
  } \
  \
  A& operator=(const A& other) { \
    return operator=(other.c_str()); \
  } \
  \
  template <size_t S> \
  A& operator=(const str_fixed<S>& other) { \
    return operator=(other.c_str()); \
  } \
  \
  template <size_t S> \
  A& operator=(const str_dynamic<S>& other) { \
    return operator=(other.c_str()); \
  } \
  \
  A& operator+=(const char* other) { \
    return append(other); \
  } \
  \
  template <size_t S> \
  A& operator+=(const str_fixed<S>& other) { \
    return append(other.c_str()); \
  } \
  template <size_t S> \
  A& operator+=(const str_dynamic<S>& other) { \
    return append(other.c_str()); \
  } \
  \
  bool operator==(const char *other) const { \
    return compare(other) == 0; \
  } \
  \
  template <size_t S> \
  bool operator==(const str_fixed<S>& other) const { \
    return compare(other.c_str()) == 0; \
  } \
  template <size_t S> \
  bool operator==(const str_dynamic<S>& other) const { \
    return compare(other.c_str()) == 0; \
  } \
  \
  bool operator!=(const char *other) const { \
    return compare(other) != 0; \
  } \
  template <size_t S> \
  bool operator!=(const str_fixed<S>& other) const { \
    return compare(other.c_str()) != 0; \
  } \
  template <size_t S> \
  bool operator!=(const str_dynamic<S>& other) const { \
    return compare(other.c_str()) != 0; \
  } \
  \
  char& operator[](const size_t pos) { \
    return at(pos); \
  } \
  \
  char& at(const size_t pos) { \
    return buffer[pos]; \
  } \
  \
  size_t used() const { \
    return _used; \
  } \
  \
  bool empty() const { \
    return _used == 0; \
  } \
  \
  const char* c_str() const { \
    return buffer; \
  }

// Forward declaring in order to reference it in `str_fixed`
template <size_t N>
class str_dynamic;

template <size_t N>
class str_fixed {
  char buffer[N];
  size_t _used;

public:
  static const std::size_t npos = static_cast<std::size_t>(-1);

  STRING_COMMON_ITERATOR(str_fixed)

  str_fixed() : _used(0) {
    buffer[0] = '\0';
  }

  str_fixed(const char* other) : str_fixed() {
    operator=(other);
  }

  template <size_t S>
  str_fixed(const str_fixed<S>& other) : str_fixed(other.c_str()) { }

  template <size_t S>
  str_fixed(const str_dynamic<S>& other) : str_fixed(other.c_str()) { }

  str_fixed& operator=(const char *other) {
    size_t len = strlen(other) + 1;
    if(len > N) len = N;
    memcpy(buffer, other, len < N ? len : N); 
    buffer[len - 1] = '\0';
    _used = len;

    return *this;
  }

  str_fixed& insert(const size_t pos, const char* other, const size_t sub_pos, const size_t len) {
    size_t available = N - _used;
    if(!_used) available--;

    if(!len || !available || pos > (_used ? _used - 1 : _used)) return *this;

    const char* other_ptr = &(other[sub_pos]);
    size_t length = strlen(other_ptr);

    if(!length) return *this;

    if(len < length) length = len;

    if(available < length) length = available;

    if(_used && pos < _used - 1)
      memmove(&buffer[pos + length], &buffer[pos], _used - pos);

    if(other == buffer) {
      size_t copied = 0;
      if(sub_pos < pos + length) {
        copied = pos - sub_pos;
        if(length < copied) copied = length; 

        memcpy(&buffer[pos], &buffer[sub_pos], copied);
      }

      if(copied < length)
        memcpy(&buffer[pos + copied], &buffer[pos + length], length - copied);
    } else {
      memcpy(&buffer[pos], other_ptr, length);
    }

    if(!_used) _used++;
    _used += length;

    buffer[_used - 1] = '\0';

    return *this;
  }

  str_fixed substr(const size_t pos = 0, const size_t len = npos) const {
    str_fixed copy;
    if(pos >= _used || !len) return copy;

    size_t length = N - pos;
    if(len < length) length = len;

    copy.append(&buffer[pos], length);

    return copy;
  }

  size_t size() const {
    return N;
  }

  STRING_COMMON_METHODS(str_fixed);
};

template <size_t N>
std::ostream& operator<<(std::ostream& os, const str_fixed<N>& str) {
  os << str.c_str();

  return os;
}

template <size_t N>
class str_dynamic {
  #ifdef ARENA_POOL_CPP
  apc::arena* _arena;
  #endif
  char* buffer;
  size_t _used;
  char static_buffer[N];
  size_t _size;

  void maybe_grow(const size_t size, const bool init) {
    if(!size) return;

    if(init && size <= N) return;

    size_t new_size = _used + size;
    if(!_used && !init) new_size++;

    if(new_size <= _size) return;

    if(!init) {
      size_t double_size = ((_size) * 2);
      if(new_size < double_size) new_size = double_size;
    }

    resize(new_size);
  }

  void moved_reset() {
    buffer = static_buffer;
    static_buffer[0] = '\0';
    _size = N;
    _used = 0;
  }

public:
  static const std::size_t npos = static_cast<std::size_t>(-1);

  STRING_COMMON_ITERATOR(str_dynamic)

  str_dynamic(size_t size = N) :
    #ifdef ARENA_POOL_CPP
    _arena(nullptr),
    #endif
    buffer(static_buffer),
    _used(0),
    static_buffer{'\0'},
    _size(N)
  {
    maybe_grow(size, true);
  }

  str_dynamic(const str_dynamic& other) :
    str_dynamic(other.buffer, other._size) { }

  // Move constructor
  // (str_dynamic only)
  str_dynamic(str_dynamic&& other) : str_dynamic()
  {
    #ifdef ARENA_POOL_CPP
    if(other._arena) _arena = other._arena;
    #endif
    
    if(other.buffer == other.static_buffer &&
      other._size
    )
      strncpy(buffer, other.buffer, other._size);
    else
      buffer = other.buffer;

    _size = other._size;
    _used = other._used;
    other.moved_reset();
  }

  str_dynamic(const char* other, const size_t size = N) : str_dynamic(size)
  {
    operator=(other);
  }

  template <size_t S>
  str_dynamic(const str_dynamic<S>& other, const size_t size = N) :
    str_dynamic(other.c_str(), size) { }

  template <size_t S>
  str_dynamic(const str_fixed<S>& other, const size_t size = N) :
    str_dynamic(other.c_str(), size) { }

  #ifdef ARENA_POOL_CPP
  str_dynamic(apc::arena &arena, size_t size = N) :
    _arena(&arena),
    buffer(static_buffer),
    _used(0),
    static_buffer{'\0'},
    _size(N)
  {
    maybe_grow(size, true);
  }

  str_dynamic(apc::arena &arena, const char* other, const size_t size = N) :
    str_dynamic(arena, size)
  {
    append(other);
  }

  template <size_t S>
  str_dynamic(apc::arena &arena, const str_dynamic<S>& other, const size_t size = N) : str_dynamic(arena, other.c_str(), size) { }

  template <size_t S>
  str_dynamic(apc::arena &arena, const str_fixed<S>& other, const size_t size = N) : str_dynamic(arena, other.c_str(), size) { }

  #endif

  ~str_dynamic() {
    if(
      #ifdef ARENA_POOL_CPP
      _arena == nullptr &&
      #endif
      buffer != static_buffer
    ) free(buffer);
  }

  str_dynamic& insert(const size_t pos, const char* other, const size_t sub_pos, const size_t len) {
    if(!len || pos > _used - 1) return *this;

    const char* other_ptr = &(other[sub_pos]);
    size_t length = strlen(other_ptr);
    const bool is_self = other == buffer;

    if(!length) return *this;

    if(len < length) length = len;

    maybe_grow(length, false);

    if(_used && pos < _used - 1)
      memmove(&buffer[pos + length], &buffer[pos], _used - pos);

    if(is_self) {
      other_ptr = &buffer[sub_pos];
      size_t copied = 0;
      if(sub_pos < pos + length) {
        copied = pos - sub_pos;
        if(length < copied) copied = length; 

        memcpy(&buffer[pos], &buffer[sub_pos], copied);
      }

      if(copied < length)
        memcpy(&buffer[pos + copied], &buffer[pos + length], length - copied);
    } else {
      memcpy(&buffer[pos], other_ptr, length);
    }

    if(!_used) _used++;
    _used += length;

    buffer[_used - 1] = '\0';

    return *this;
  }

  str_dynamic& resize(const size_t size) {
    if(size == _size) return *this;

    if(size <= N) {
      if(buffer == static_buffer) return *this;

      if(N < _used) _used = N;

      memcpy(static_buffer, buffer, _used); 
      
      #ifdef ARENA_POOL_CPP
      if(!_arena) free(buffer);
      #else
      free(buffer);
      #endif

      buffer = static_buffer;
      buffer[_used - 1] = '\0';
      _size = N;
    } else {
      if(buffer == static_buffer) {
        char* new_buffer = nullptr;

        #ifdef ARENA_POOL_CPP
        if(_arena) new_buffer = _arena->allocate_size<char>(size);
        else
          new_buffer = static_cast<char *>(malloc(sizeof(char) * size));
        #else
        new_buffer = static_cast<char *>(malloc(sizeof(char) * size));
        #endif

        if(new_buffer) {
          memcpy(new_buffer, buffer, _used);
          buffer = new_buffer;
          _size = size;
        }
      } else {
        char* new_buffer = nullptr;

        #ifdef ARENA_POOL_CPP
        if(_arena) {
          new_buffer = _arena->allocate_size<char>(size);

          if(new_buffer) {
            memcpy(new_buffer, buffer, _used < size ? _used : size);
          }
        } else {
          new_buffer = static_cast<char *>(
            realloc(buffer, sizeof(char) * size)
          );
        }
        #else
        new_buffer = static_cast<char *>(
          realloc(buffer, sizeof(char) * size)
        );
        #endif

        if(new_buffer) {
          if(_used > size) {
            _used = size;
            new_buffer[_used - 1] = '\0';
          }

          buffer = new_buffer;
          _size = size;
        }
      }
    }

    return *this;
  }

  str_dynamic& operator=(const char* other) {
    _used = 0;
    buffer[0] = '\0';

    return append(other);
  }

  str_dynamic substr(const size_t pos = 0, const size_t len = npos) const {
    str_dynamic copy;
    if(pos >= _used || !len) return copy;

    size_t length = strlen(&buffer[pos]);
    if(len < length) length = len;

    copy.append(&buffer[pos], length);

    return copy;
  }

  void shrink_to_fit() {
    #ifdef ARENA_POOL_CPP
    if(_arena) return;
    #endif

    if(_size <= 1 || _used == _size) return;

    resize(_used);
  }

  size_t size() const {
    return _size;
  }

  STRING_COMMON_METHODS(str_dynamic);
};

template <size_t N>
std::ostream& operator<<(std::ostream& os, const str_dynamic<N>& str) {
  os << str.c_str();

  return os;
}

typedef str_fixed<4> str4; 

typedef str_fixed<8> str8; 

typedef str_fixed<16> str16; 

typedef str_fixed<32> str32; 

typedef str_fixed<64> str64; 

typedef str_fixed<128> str128; 

typedef str_fixed<256> str256; 

typedef str_dynamic<32> str;

}
