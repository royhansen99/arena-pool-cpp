/*
 * Package: arena_pool_cpp
 * Version: 0.3.4
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
#include "./vector.h"
#include "./pool.h"
#include "./rapidhash.h"

namespace apc {

template <typename T, size_t S>
struct hashmap_item {
  char key[S];
  T current;
  hashmap_item<T, S>* next = nullptr;
};

#define HASHMAP_CLASS_COMMON(ITERATOR) \
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
  class iterator { \
    ITERATOR it; \
  public: \
    explicit iterator(const ITERATOR&& begin): \
      it(begin) { } \
 \
    T& operator*() const { return (*it).current; } \
    \
    bool operator!=(const iterator& other) const { return it != other.it; } \
    \
    bool operator!=(const void* other) const { return it != other; } \
    \
    iterator& operator+(size_t count) { \
      it + count; \
      \
      return *this; \
    } \
    \
    iterator& operator++() { \
      ++it; \
      \
      return *this; \
    } \
  }; \
 \
  iterator begin() const { \
    return iterator(pool.begin()); \
  } \
 \
  iterator rbegin() const { \
    return iterator(pool.rbegin()); \
  } \
 \
  const iterator end() const { \
    return iterator(pool.end()); \
  } \
 \
  const iterator rend() const { \
    return iterator(pool.rend()); \
  } \
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
  } \
  bool erase(iterator it) { \
    if(!(it != nullptr)) return false; \
    \
    return erase( \
      (reinterpret_cast<hashmap_item<T, S>*>( \
        &(*it)) \
      )->key \
    ); \
  }

template <typename T, size_t S, size_t Z, bool FORCE_TRIVIAL_COPY = false>
class hashmap_fixed {
private:
  apc::vector_fixed<hashmap_item<T, S>*, Z> array;
  apc::pool_fixed<hashmap_item<T, S>, Z> pool;

  using target_iterator = typename apc::pool_fixed<hashmap_item<T, S>, Z>::iterator;
  HASHMAP_CLASS_COMMON(target_iterator)

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
        if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
          memcpy(&current->current, &item, sizeof(T));
        } else {
          if(!std::is_trivially_destructible<T>::value)
            current->current.~T();

          new (&current->current) T(item);
        }

        return true;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return false;

    memcpy(n->key, key, S);

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
      memcpy(&n->current, &item, sizeof(T));
    else
      new (&n->current) T(item);

    n->next = *location;

    *location = n;

    if(n->next == nullptr) _used++;

    return true;
  }
};

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY = false>
class hashmap {
private:
  #ifdef ARENA_POOL_CPP
  apc::arena* _arena = nullptr;
  #endif

  apc::vector<hashmap_item<T, S>*> array;
  apc::pool<hashmap_item<T, S>> pool;

  using target_iterator = typename apc::pool<hashmap_item<T, S>>::iterator;
  HASHMAP_CLASS_COMMON(target_iterator)

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
        if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
          memcpy(&current->current, &item, sizeof(T));
        } else {
          if(!std::is_trivially_destructible<T>::value)
            current->current.~T();

          new (&current->current) T(item);
        }

        return true;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return false;

    memcpy(n->key, key, S);

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
      memcpy(&n->current, &item, sizeof(T));
    else
      new (&n->current) T(item);

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
