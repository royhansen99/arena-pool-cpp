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

#define HASHMAP_CLASS_COMMON(TYPE, ITERATOR) \
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
  template <size_t X, size_t Y> \
  TYPE(hashmap_fixed<T, X, Y, FORCE_TRIVIAL_COPY>& other) : TYPE() { \
    operator=(other); \
  } \
  \
  template <size_t X, size_t Y> \
  TYPE& operator=(hashmap_fixed<T, X, Y, FORCE_TRIVIAL_COPY>& other) { \
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
  TYPE(hashmap<T, X, FORCE_TRIVIAL_COPY>& other) : TYPE() { \
    operator=(other); \
  } \
  \
  template <size_t X> \
  TYPE& operator=(hashmap<T, X, FORCE_TRIVIAL_COPY>& other) { \
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
  size_t size() { return pool.size(); } \
  \
  size_t used() { return _used; } \
  \
  void reset() { \
    _reset(true); \
  } \
  \
  T* insert(void* key, T& item) { \
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
  T* insert(void* key, T&& item) { \
    return insert(key, item); \
  } \
  \
  T* find(void* key) { \
    uint64_t hash = rapidhashNano(key, S); \
    size_t index = hash % (array.size() - 1); \
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
    if(!_used) return false; \
    \
    uint64_t hash = rapidhashNano(key, S); \
    size_t index = hash % (array.size() - 1); \
    \
    hashmap_item<T, S>* prev = nullptr; \
    auto* current = array[index]; \
    \
    while(current != nullptr) { \
      if(memcmp(current->key, key, S) == 0) { \
        if(prev != nullptr)\
          prev->next = current->next; \
        else \
          array[index] = current->next; \
        \
        _used--; \
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

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY = false>
class hashmap;

template <typename T, size_t S, size_t Z, bool FORCE_TRIVIAL_COPY = false>
class hashmap_fixed {
private:
  apc::vector_fixed<hashmap_item<T, S>*, Z> array;
  apc::pool_fixed<hashmap_item<T, S>, Z> pool;

  T* insert_raw(void* key) {
    if(!size()) return nullptr;

    uint64_t hash = rapidhashNano(key, S);
    size_t index = hash % (array.size() - 1);      

    auto** location = &array[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, key, S) == 0) {
        if(!std::is_trivially_destructible<T>::value &&
          !FORCE_TRIVIAL_COPY)
          current->current.~T();

        return &current->current;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return nullptr;

    memcpy(n->key, key, S);

    n->next = *location;

    *location = n;

    _used++;

    return &n->current;
  }

  using target_iterator = typename apc::pool_fixed<hashmap_item<T, S>, Z>::iterator;
  HASHMAP_CLASS_COMMON(hashmap_fixed, target_iterator)

public:
  hashmap_fixed() {
    _reset();
  }
};

template <typename T, size_t S, bool FORCE_TRIVIAL_COPY>
class hashmap {
private:
  #ifdef ARENA_POOL_CPP
  apc::arena* _arena = nullptr;
  #endif

  apc::vector<hashmap_item<T, S>*> array;
  apc::pool<hashmap_item<T, S>> pool;

  T* insert_raw(void* key) {
    if(!size()) init();
    if(!size()) return nullptr;

    uint64_t hash = rapidhashNano(key, S);
    size_t index = hash % (array.size() - 1);      

    auto** location = &array[index];
    auto* current = *location;

    while(current != nullptr) {
      if(memcmp(current->key, key, S) == 0) {
        if(!std::is_trivially_destructible<T>::value &&
          !FORCE_TRIVIAL_COPY)
          current->current.~T();

        return &current->current;
      }

      current = current->next;
    }

    apc::hashmap_item<T, S>* n = pool.allocate_raw();

    if(n == nullptr) return nullptr;

    memcpy(n->key, key, S);

    n->next = *location;

    *location = n;

    _used++;

    if(n->next == nullptr) {
      float factor = (float)_used / (float)array.size();

      if(factor >= 0.75) {
        size_t new_size = array.size() * 2;

        array.reset(); // reset array before resize, to prevent copying old pointers, because we dont need them. 
        array.resize(new_size);

        _reset();

        auto* pool_item = pool.used_ptr();

        while(pool_item != nullptr) {
          uint64_t hash = rapidhashNano(&pool_item->value.key, S);
          size_t index = hash % (array.size() - 1);

          auto* location = array[index];

          pool_item->value.next = location;

          array[index] = &pool_item->value;

          pool_item = pool_item->next;
        }
      }
    }

    return &n->current;
  }

  using target_iterator = typename apc::pool<hashmap_item<T, S>>::iterator;
  HASHMAP_CLASS_COMMON(hashmap, target_iterator)

public:
  hashmap(size_t size = 0) {
    if(!size) return;

    init(size);
  }

  hashmap(hashmap&& other) {
    #ifdef ARENA_POOL_CPP
    if(other._arena) _arena = other._arena;
    #endif

    array = std::move(other.array);
    pool = std::move(other.pool);
    _used = other._used;

    other._used = 0;
  }

  hashmap& operator=(hashmap&& other) {
    #ifdef ARENA_POOL_CPP
    if(other._arena) _arena = other._arena;
    #endif

    array = std::move(other.array);
    pool = std::move(other.pool);
    _used = other._used;

    other._used = 0;

    return *this;
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
};

};
