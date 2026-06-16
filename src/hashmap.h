/*
 * Package: arena_pool_cpp
 * Version: 0.2.7
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
#include "./vector.h"
#include "./pool.h"
#include "./rapidhash.h"

#ifdef ARENA_POOL_CPP
#include "./arena.h"
#endif

namespace apc {

template <typename T, size_t S>
struct hashmap_item {
  char key[S];
  T current;
  hashmap_item<T, S>* next = nullptr;
};

#define HASHMAP_CLASS_COMMON \
  private: \
  size_t _used = 0; \
  \
  void _reset(bool reset_pool = false) { \
    for(size_t i = 0; i < array.size(); i++) { \
      array[i] = nullptr; \
    } \
    \
    _used = 0; \
    \
    if(reset_pool) pool.reset(); \
  } \
  \
  public: \
  \
  size_t size() { return array.size(); } \
  \
  size_t used() { return _used; } \
  \
  void reset() { \
    _reset(true); \
  } \
  \
  bool insert(T&& item, void* key) { \
    return insert(item, key); \
  } \
  \
  T* find(void* key) { \
    uint64_t hash = rapidhashNano(key, S); \
    size_t index = hash % (size() - 1); \
    \
    auto* current = array[index]; \
    \
    while(current != nullptr) { \
      if(memcmp(current->key, key, S) == 0) \
        return &current->current; \
      \
      current = current->next; \
    } \
    \
    return nullptr; \
  } \
  \
  bool erase(void* key) { \
    if(!size()) return false; \
    \
    uint64_t hash = rapidhashNano(key, S); \
    size_t index = hash % (size() - 1); \
    \
    hashmap_item<T, S>* prev = nullptr; \
    auto* current = array[index]; \
    \
    while(current != nullptr) { \
      if(memcmp(current->key, key, S) == 0) { \
        if(prev != nullptr) { \
          prev->next = current->next; \
        } else { \
          array[index] = current->next; \
          \
          if(current->next == nullptr) \
            _used--; \
        } \
        \
        pool.deallocate(current); \
        \
        return true; \
      } \
      \
      prev = current; \
      current = current->next; \
    } \
    \
    return false; \
  }


template <typename T, size_t S, size_t Z>
class hashmap_fixed {
private:
  apc::vector_fixed<hashmap_item<T, S>*, Z> array;
  apc::pool_fixed<hashmap_item<T, S>, Z> pool;

  HASHMAP_CLASS_COMMON

public:
  hashmap_fixed() {
    _reset();
  }

  bool insert(T& item, void* key) {
    if(!size()) return false;

    uint64_t hash = rapidhashNano(key, S);
    size_t index = hash % (size() - 1);      

    auto** location = &array[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, key, S) == 0) {
        memcpy(&current->current, &item, sizeof(T));
        return true;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return false;

    memcpy(n->key, key, S);
    memcpy(&n->current, &item, sizeof(T));
    n->next = *location;

    *location = n;

    if(n->next == nullptr) _used++;

    return true;
  }
};

template <typename T, size_t S>
class hashmap {
private:
  #ifdef ARENA_POOL_CPP
  apc::arena* _arena = nullptr;
  #endif

  apc::vector<hashmap_item<T, S>*> array;
  apc::pool<hashmap_item<T, S>> pool;

  HASHMAP_CLASS_COMMON

public:
  hashmap(size_t size = 0) {
    if(!size) return;

    init(size);
  }

  void init(size_t _size = 16) {
    // If requested size is 0, or already initialized, return.
    if(!_size || size()) return;

    array.init(_size);
    pool.init(_size);
    _reset();
  }

  #ifdef ARENA_POOL_CPP
  hashmap(apc::arena& arena, size_t size = 16) : _arena(&arena) {
    init(arena, size);
  }

  void init(apc::arena& arena, size_t _size = 16) {
    // If requested size is 0, or already initialized, return.
    if(!_size || size()) return;

    array.init(arena, _size);
    pool.init(arena, _size);
    _reset();
  }
  #endif

  bool insert(T& item, void* key) {
    if(!size()) init();
    if(!size()) return false;

    uint64_t hash = rapidhashNano(key, S);
    size_t index = hash % (size() - 1);      

    auto** location = &array[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, key, S) == 0) {
        memcpy(&current->current, &item, sizeof(T));
        return true;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return false;

    memcpy(n->key, key, S);
    memcpy(&n->current, &item, sizeof(T));
    n->next = *location;

    *location = n;

    if(n->next == nullptr) {
      _used++;

      float factor = (float)_used / (float)size();

      if(factor >= 0.75) {
        size_t new_size = size() * 2;

        array.reset(); // reset array before resize, to prevent copying old pointers, because we dont need them. 
        array.resize(new_size);

        _reset();

        auto* pool_item = pool.used_ptr();

        while(pool_item != nullptr) {
          uint64_t hash = rapidhashNano(&pool_item->value.key, S);
          size_t index = hash % (size() - 1);

          auto* location = array[index];

          if(location == nullptr) _used++;

          pool_item->value.next = location;

          array[index] = &pool_item->value;

          pool_item = pool_item->next;
        }
      }
    }

    return true;
  }
};

};
