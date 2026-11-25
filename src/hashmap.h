/*
 * Package: arena_pool_cpp
 * Version: 0.4.0
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast memory allocator library with an arena/bump-allocator,
 *              a pool-allocator, a hashmap-allocator,  and a vector-like
 *              array-allocator. Also includes a string implementation.
 *              Single header c++11 library.
 */
#pragma once

#include <cstddef>
#include <type_traits>
#include <cstring>
#include <utility>
#include "./rapidhash.h"

// Helper for clearing padding when using struct keys. Should work on
// most modern platforms (linux, Windows, macOS).
// If you attempt to use it when your compiler does not support it,
// you will get an error.
#if defined(__has_builtin) && __has_builtin(__builtin_clear_padding)
  #define APC_CLR_PAD(ptr) __builtin_clear_padding(ptr)
#elif defined(__GNUC__) && (__GNUC__ >= 11)
  #define APC_CLR_PAD(ptr) __builtin_clear_padding(ptr)
#elif defined(_MSC_VER)
  #define APC_CLR_PAD(ptr) __builtin_zero_non_value_bits(ptr)
#else
  #define APC_CLR_PAD(ptr) \
    assert(false && "APC_CLR_PAD called on unsupported compiler"); 
  #define APC_CLR_PAD_MISSING
#endif

namespace apc {

template <typename T, size_t S>
struct hashmap_item {
  char key[S];
  T current;
  hashmap_item<T, S>* hash_next = nullptr;
  hashmap_item<T, S>* next = nullptr;
  hashmap_item<T, S>* prev = nullptr;
};

template <typename T, size_t S>
struct hashmap_page {
  hashmap_item<T, S>* buffer;
  size_t size;
};

#define HASHMAP_CLASS_COMMON(TYPE) \
  private: \
  hashmap_item<T, S>* free_ptr = nullptr; \
  hashmap_item<T, S>* first = nullptr; \
  hashmap_item<T, S>* last = nullptr; \
  size_t _used = 0; \
  size_t _size = 0; \
  \
  void reset_keys() { \
    for(size_t i = 0; i < keys_size; i++) { \
      keys[i] = nullptr; \
    } \
  } \
  \
  void reset_pool() { \
    auto* next = first; \
    while(next != nullptr) { \
      auto* _next = next->next; \
      \
      if(!std::is_trivially_destructible<T>::value && \
        !FORCE_TRIVIAL_COPY) \
        next->current.~T(); \
      \
      next->next = free_ptr; \
      free_ptr = next; \
      \
      next = _next; \
    } \
    \
    _used = 0; \
    first = nullptr; \
    last = nullptr; \
  } \
  public: \
  class iterator { \
    hashmap_item<T, S>* it; \
    bool reverse; \
  public: \
    explicit iterator(hashmap_item<T, S>* begin, bool _reverse = false): \
      it(begin), reverse(_reverse) { } \
    \
    T& operator*() const { return it->current; } \
    \
    bool operator!=(const iterator& other) const { return it != other.it; } \
    \
    bool operator!=(const void* other) const { return it != other; } \
    \
    iterator& operator+(size_t count) { \
      if(it == nullptr) return *this; \
      \
      if(reverse) { \
        it = it->next; \
      } else { \
        it = it->prev; \
      } \
      return *this; \
    } \
    \
    iterator& operator++() { \
      operator+(1); \
      \
      return *this; \
    } \
  }; \
  \
  iterator begin() const { \
    return iterator(last); \
  } \
  \
  iterator rbegin() const { \
    return iterator(first, true); \
  } \
  \
  const iterator end() const { \
    return iterator(nullptr); \
  } \
  \
  const iterator rend() const { \
    return iterator(nullptr); \
  } \
  \
  template <size_t X, size_t Y> \
  TYPE(hashmap_fixed<T, X, Y, FORCE_TRIVIAL_COPY, STRING_KEY>& other) : TYPE() { \
    operator=(other); \
  } \
  \
  template <size_t X, size_t Y> \
  TYPE& operator=(hashmap_fixed<T, X, Y, FORCE_TRIVIAL_COPY, STRING_KEY>& other) { \
    reset(); \
    \
    using ItemType = hashmap_item<T, S>; \
    for(auto& i : other) { \
      auto* item = reinterpret_cast<hashmap_item<T, S>*>( \
        reinterpret_cast<char*>(&i) - \
        offsetof(ItemType, current) \
      ); \
      \
      if(!insert(item->key, item->current)) \
        break; \
    } \
    return *this; \
  } \
  \
  template <size_t X> \
  TYPE(hashmap<T, X, FORCE_TRIVIAL_COPY, STRING_KEY>& other) : TYPE() { \
    operator=(other); \
  } \
  \
  template <size_t X> \
  TYPE& operator=(hashmap<T, X, FORCE_TRIVIAL_COPY, STRING_KEY>& other) { \
    reset(); \
    \
    using ItemType = hashmap_item<T, S>; \
    for(auto& i : other) { \
      auto* item = reinterpret_cast<hashmap_item<T, S>*>( \
        reinterpret_cast<char*>(&i) - \
        offsetof(ItemType, current) \
      ); \
      \
      if(!insert(item->key, item->current)) \
        break; \
    } \
    return *this; \
  } \
  \
  size_t size() { \
    return _size; \
  } \
  \
  size_t used() { return _used; } \
  \
  void reset() { \
    reset_keys(); \
    reset_pool(); \
  } \
  \
  T* insert(const void* key, T& item) { \
    auto* target = insert_raw(key); \
    \
    if(target == nullptr) return nullptr; \
    \
    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) \
      memcpy(target, &item, sizeof(T)); \
    else \
      new (target) T(item); \
    \
    return target; \
  } \
  \
  template <typename... Args> \
  T* insert_new(void* key, Args&&... args) { \
    auto* target = insert_raw(key); \
    \
    if(target == nullptr) return nullptr; \
    \
    new (target) T(std::forward<Args>(args)...);  \
    \
    return target; \
  } \
  \
  T* insert(const void* key, T&& item) { \
    return insert(key, item); \
  } \
  \
  T* find(const void* key) { \
    char _key[S]; \
    if(STRING_KEY) strncpy(_key, static_cast<const char*>(key), S); \
    \
    uint64_t hash = rapidhashNano(STRING_KEY ? _key : key, S); \
    size_t index = hash % (keys_size - 1); \
    \
    auto* current = keys[index]; \
    \
    while(current != nullptr) { \
      if(memcmp(current->key, STRING_KEY ? _key : key, S) == 0) \
        return &current->current; \
      \
      current = current->hash_next; \
    } \
    \
    return nullptr; \
  } \
  \
  bool erase(const void* key) { \
    if(!_used) return false; \
    \
    char _key[S]; \
    if(STRING_KEY) strncpy(_key, static_cast<const char*>(key), S); \
    \
    uint64_t hash = rapidhashNano(STRING_KEY ? _key : key, S); \
    size_t index = hash % (keys_size - 1); \
    \
    hashmap_item<T, S>* prev = nullptr; \
    auto* current = keys[index]; \
    \
    while(current != nullptr) { \
      if(memcmp(current->key, STRING_KEY ? _key : key, S) == 0) { \
        if(prev != nullptr)\
          prev->hash_next = current->hash_next; \
        else \
          keys[index] = current->hash_next; \
        \
        _used--; \
        \
        if(!std::is_trivially_destructible<T>::value) current->current.~T(); \
        \
        if(current->next) current->next->prev = current->prev; \
        if(current->prev) current->prev->next = current->next; \
        if(last == current) last = current->prev; \
        if(first == current) first = current->next; \
        \
        current->next = free_ptr; \
        free_ptr = current; \
        \
        return true; \
      } \
      \
      prev = current; \
      current = current->hash_next; \
    } \
    \
    return false; \
  } \
  \
  bool erase(iterator it) { \
    if(!(it != nullptr)) return false; \
    \
    return erase( \
      (reinterpret_cast<hashmap_item<T, S>*>( \
        &(*it)) \
      )->key \
    ); \
  }

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY = false, bool STRING_KEY = false>
class hashmap;

template <typename T, size_t S, size_t Z, bool FORCE_TRIVIAL_COPY = false, bool STRING_KEY = false>
class hashmap_fixed {
private:
  hashmap_item<T, S>* keys[Z];
  size_t keys_size = Z;
  hashmap_item<T, S> buffer[Z];

  hashmap_item<T, S>* allocate() {
    if(!free_ptr) return nullptr;

    auto* current = free_ptr;
    free_ptr = current->next;

    current->hash_next = nullptr;
    current->prev = nullptr;
    current->next = first;

    if(first) first->prev = current;

    first = current;

    if(!last) last = current;

    return current;
  }

  T* insert_raw(const void* key) {
    if(!size()) return nullptr;

    char _key[S];
    if(STRING_KEY) strncpy(_key, static_cast<const char*>(key), S);

    uint64_t hash = rapidhashNano(STRING_KEY ? _key : key, S);
    size_t index = hash % (keys_size - 1);      

    auto** location = &keys[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, STRING_KEY ? _key : key, S) == 0) {
        if(!std::is_trivially_destructible<T>::value &&
          !FORCE_TRIVIAL_COPY)
          current->current.~T();

        return &current->current;
      }

      current = current->next;
    }

    auto* n = allocate();

    if(n == nullptr) return nullptr;

    memcpy(n->key, STRING_KEY ? _key : key, S);

    n->next = *location;

    *location = n;

    _used++;

    return &n->current;
  }

  HASHMAP_CLASS_COMMON(hashmap_fixed)

public:
  hashmap_fixed() : _size(Z) {
    reset_keys();

    for(size_t i = 0; i < Z; i++) {
      buffer[i].next = free_ptr;
      free_ptr = &buffer[i];
    }
  }
};

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY, bool STRING_KEY>
class hashmap {
private:
  #ifdef APC_ARENA
  apc::arena* _arena = nullptr;
  #endif

  hashmap_item<T, S>** keys = nullptr;
  size_t keys_size = 0;
  hashmap_page<T, S>** pages = nullptr;
  size_t pages_size = 0;

  hashmap_item<T, S>* allocate() {
    if(!free_ptr && !add_page(size() ? size() : 1)) return nullptr;

    auto* current = free_ptr;
    free_ptr = current->next;

    current->hash_next = nullptr;
    current->prev = nullptr;
    current->next = first;

    if(first) first->prev = current;

    first = current;

    if(!last) last = current;

    return current;
  }

  T* insert_raw(const void* key) {
    if(!size()) init();
    if(!size()) return nullptr;

    char _key[S];
    if(STRING_KEY) strncpy(_key, static_cast<const char*>(key), S);

    uint64_t hash;

    hash = rapidhashNano(STRING_KEY ? _key : key, S);
    size_t index = hash % (keys_size - 1);

    auto** location = &keys[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, STRING_KEY ? _key : key, S) == 0) {
        if(!std::is_trivially_destructible<T>::value &&
          !FORCE_TRIVIAL_COPY)
          current->current.~T();

        return &current->current;
      }

      current = current->hash_next;
    }

    auto* n = allocate();

    if(n == nullptr) return nullptr;

    memcpy(n->key, STRING_KEY ? _key : key, S);

    n->hash_next = *location;

    *location = n;

    _used++;

    float factor = (float)_used / (float)keys_size;

    if(factor >= 0.75) {
      if(resize_keys(keys_size * 2)) {
        reset_keys();

        auto* next = first;

        while(next != nullptr) {
          uint64_t hash = rapidhashNano(&next->key, S);
          size_t index = hash % (keys_size - 1);

          next->hash_next = keys[index];

          keys[index] = next;

          next = next->next;
        }
      }
    }

    return &n->current;
  }

  bool add_page(size_t _size) {
    char* new_buffer;

    #ifdef APC_ARENA
    if(_arena) {
      new_buffer = static_cast<char*>(
        _arena->allocate_raw(
          sizeof(hashmap_page<T, S>) +
          (sizeof(hashmap_page<T, S>*) * (pages_size + 1)) +
          alignof(hashmap_page<T, S>*) +
          (sizeof(hashmap_item<T, S>) * _size) +
          alignof(hashmap_item<T, S>)
        )
      );
    } else {
    #endif
      new_buffer = static_cast<char*>(
        malloc(
          sizeof(hashmap_page<T, S>) +
          (sizeof(hashmap_page<T, S>*) * (pages_size + 1)) +
          alignof(hashmap_page<T, S>*) +
          (sizeof(hashmap_item<T, S>) * _size) +
          alignof(hashmap_item<T, S>)
        )
      );
    #ifdef APC_ARENA
    }
    #endif

    if(!new_buffer) return false;

    char* position = new_buffer;

    auto* new_page = reinterpret_cast<hashmap_page<T, S>*>(position);
    position += sizeof(hashmap_page<T, S>);

    {
      uintptr_t current_addr = reinterpret_cast<uintptr_t>(position);
      size_t alignment = alignof(hashmap_page<T, S>*);
      position += (alignment - (current_addr % alignment)) % alignment;
    }

    auto* new_pages = reinterpret_cast<hashmap_page<T, S>**>(position);
    position += (sizeof(hashmap_page<T, S>*) * (pages_size + 1));

    if(pages_size)
      memcpy(new_pages, pages, sizeof(hashmap_page<T, S>*) * pages_size);

    {
      uintptr_t current_addr = reinterpret_cast<uintptr_t>(position);
      size_t alignment = alignof(hashmap_item<T, S>);
      position += (alignment - (current_addr % alignment)) % alignment;
    }

    new_page->size = _size;
    new_page->buffer = reinterpret_cast<hashmap_item<T, S>*>(position);

    for(size_t i = 0; i < _size; i++) {
      new_page->buffer[i].next = free_ptr;
      free_ptr = &new_page->buffer[i];
    }

    new_pages[pages_size] = new_page;
    pages = new_pages;
    pages_size++;
    this->_size += new_page->size;

    return true;
  }

  bool resize_keys(size_t _size) {
    hashmap_item<T, S>** new_keys;

    #ifdef APC_ARENA
    if(_arena) {
      new_keys = _arena->allocate_size<hashmap_item<T, S>*>(_size);
    } else {
    #endif
    new_keys = static_cast<hashmap_item<T, S>**>(
      malloc(sizeof(hashmap_item<T, S>*) * _size)
    );
    #ifdef APC_ARENA
    }
    #endif

    if(!new_keys) return false;

    #ifdef APC_ARENA
    if(!_arena && keys) free(keys);
    #else
    if(keys) free(keys);
    #endif

    keys = new_keys;
    keys_size = _size;

    return true;
  }

  void free_resources(bool destructor = false) {
    if(!std::is_trivially_destructible<T>::value) { 
      for(auto* current = first; current != nullptr; current = current->next)
        current->current.~T();
    }

    #ifdef APC_ARENA
    if(!_arena) {
    #endif
    if(keys) free(keys);
    if(pages) {
      for(size_t i = 0; i < pages_size; i++)
        free(pages[i]);
    }
    #ifdef APC_ARENA
    }
    #endif

    // Only zero values if this is not called from the
    // destructor.
    if(!destructor) destroy();
  }

  void destroy() {
    free_ptr = nullptr;
    first = nullptr;
    last = nullptr;

    keys = nullptr;
    keys_size = 0;
    pages = nullptr;
    pages_size = 0;
    _used = 0;
    _size = 0;
  }

  void move(hashmap&& other) {
    keys = other.keys;
    keys_size = other.keys_size;
    pages = other.pages;
    pages_size = other.pages_size;

    first = other.first;
    last = other.last;
    free_ptr = other.free_ptr;

    _used = other._used;
    _size = other._size;

    other.destroy();
  }

  HASHMAP_CLASS_COMMON(hashmap)

public:
  ~hashmap() {
    free_resources(true);
  }

  hashmap(size_t size = 0) {
    if(!size) return;

    init(size);
  }

  hashmap(hashmap&& other) {
    #ifdef APC_ARENA
    if(other._arena) _arena = other._arena;
    #endif

    move(std::move(other));
  }

  hashmap& operator=(hashmap&& other) {
    free_resources();
    move(std::move(other));

    return *this;
  }

  void init(size_t _size = 16) {
    // If requested size is 0, or already initialized, return.
    if(!_size || size()) return;

    if(!resize_keys(_size)) return;

    if(!add_page(_size)) {
      free(keys);
      keys = nullptr;
      keys_size = 0;
      return;
    }

    reset_keys();
  }

  #ifdef APC_ARENA
  hashmap(apc::arena& arena, size_t size = 16) {
    _arena = &arena;
    init(arena, size);
  }

  void init(apc::arena& arena, size_t _size = 16) {
    // If requested size is 0, or already initialized, return.
    if(!_size || size()) return;

    _arena = &arena;

    init(_size);
  }
  #endif
};

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY = false>
using hashmap_str = hashmap<T, sizeof(char[S]), FORCE_TRIVIAL_COPY, true>;

template <typename T, size_t S, size_t Z, bool FORCE_TRIVIAL_COPY = false>
using hashmap_fixed_str = hashmap_fixed<T, sizeof(char[S]), Z, FORCE_TRIVIAL_COPY, true>;

};
