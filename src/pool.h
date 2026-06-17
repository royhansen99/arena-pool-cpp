/*
 * Package: arena_pool_cpp
 * Version: 0.2.8
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast memory allocator library with an arena/bump-allocator,
 *              a pool-allocator, a hashmap-allocator,  and a vector-like
 *              array-allocator. Also includes a string implementation.
 *              Single header c++11 library.
 */
#pragma once

#include <cstdlib>
#include <cstddef>
#include <type_traits>

#ifdef ARENA_POOL_CPP
#include "./arena.h"
#endif

namespace apc {

template <typename T>
struct pool_item {
  pool_item* next; 
  pool_item* prev; 
  T value;
};


template <typename T>
struct pool_page {
  pool_item<T>* buffer;
  size_t size;
};

template <typename T, size_t S>
struct pool_page_fixed {
  pool_item<T> buffer[S];
  size_t size = S;
};

#define POOL_CLASS_COMMON \
  private: \
    pool_item<T>* free_ptr = nullptr; \
    pool_item<T>* use_ptr = nullptr; \
    size_t _used = 0; \
    \
  public: \
    size_t size() const { \
      size_t total_size = 0; \
      \
      for(size_t i = 0; i < pages_size; i++) { \
        total_size += pages[i].size; \
      } \
      \
      return total_size; \
    } \
    \
    size_t used() const { \
      return _used; \
    } \
    \
    void reset() { \
      if(!pages_size) return; \
      \
      if(!std::is_trivially_destructible<T>::value) { \
        while(use_ptr) { \
          use_ptr->value.~T(); \
          use_ptr = use_ptr->next; \
        } \
      } \
      \
      pool_item<T>* previous = nullptr; \
      for(size_t i = 0; i < pages_size; i++) { \
        for(size_t z = 0; z < pages[i].size; z++) { \
          if(previous != nullptr) previous->next = &(pages[i].buffer[z]); \
          \
          previous = &(pages[i].buffer[z]); \
          previous->prev = nullptr; \
          previous->next = nullptr; \
        } \
      } \
      \
      previous->next = nullptr; \
      free_ptr = &(pages[0].buffer[0]); \
      \
      _used = 0; \
    } \
    \
    template <typename... Args> \
    T* allocate_new(Args&&... args) { \
      T* new_item = allocate_raw(); \
      \
      if(!new_item) return nullptr; \
      \
      return new (new_item) T(std::forward<Args>(args)...); \
    } \
    \
    template <typename U> \
    T* allocate(U &&item) { \
      T* new_item = allocate_raw(); \
      \
      if(!new_item) return nullptr; \
      \
      if(std::is_trivially_copyable<T>::value) \
        memcpy(new_item, &item, sizeof(T)); \
      else \
        new (new_item) T(std::forward<U>(item)); \
      \
      return new_item; \
    } \
    \
    void deallocate(T* ptr) { \
      if(ptr == nullptr) return; \
      \
      /* Use offsetof to safely get the address of the containing PoolItem. */ \
      /* We subtract the offset of the 'value' member from the 'ptr'. */ \
      pool_item<T>* item = reinterpret_cast<pool_item<T>*>( \
        reinterpret_cast<char*>(ptr) - offsetof(pool_item<T>, value) \
      ); \
      \
      /* ptr->prev is only set if allocated, otherwise if an item is free */ \
      /* it is nullptr. */ \
      if(use_ptr != item && item->prev == nullptr) return; \
      \
      if(!std::is_trivially_destructible<T>::value) item->value.~T(); \
      \
      if(item->prev) item->prev->next = item->next; \
      if(item->next) item->next->prev = item->prev; \
      if(use_ptr == item) use_ptr = item->next; \
      \
      item->next = free_ptr; \
      item->prev = nullptr; \
      free_ptr = item; \
      \
      _used--; \
    } \
    pool_item<T>* used_ptr() { \
      return use_ptr; \
    }

template <typename T>
class pool {
private:
  #ifdef ARENA_POOL_CPP
  arena* arena = nullptr;
  #endif

  pool_page<T>* pages = nullptr;
  size_t pages_size = 0;

public:
  T* allocate_raw() {
    if(!free_ptr) {
      if(!grow(size() ? size() * 2 : 1)) return nullptr;
    }

    pool_item<T>* chunk = free_ptr;
    free_ptr = free_ptr->next;

    if(use_ptr) use_ptr->prev = chunk;

    chunk->next = use_ptr;
    chunk->prev = nullptr;
    use_ptr = chunk;
    _used++;

    return &chunk->value;
  }

  POOL_CLASS_COMMON

public:

  #ifdef ARENA_POOL_CPP
  pool(apc::arena &_arena, const size_t pool_size = 0) : arena(&_arena) {
    if(pool_size) grow(pool_size);
  }
  #endif

  pool(const size_t pool_size = 0) {
    if(pool_size) grow(pool_size);
  }

  ~pool() {
    if(!std::is_trivially_destructible<T>::value) {
      while(use_ptr) {
        use_ptr->value.~T();
        use_ptr = use_ptr->next;
      }
    }

    #ifdef ARENA_POOL_CPP
    if(!arena) {
    #endif
      for(size_t i = 0; i < pages_size; i++) {
        free(pages[i].buffer);
      }

      free(pages);
    #ifdef ARENA_POOL_CPP
    }
    #endif
  }

  void init(const size_t pool_size) {
    if(pool_size) grow(pool_size);
  }

  #ifdef ARENA_POOL_CPP
  void init(apc::arena &_arena, const size_t pool_size) {
    if(size() == 0) arena = &_arena;

    if(pool_size) grow(pool_size);
  }
  #endif

  bool grow(const size_t size) {
    pool_item<T>* new_buffer = nullptr;
    size_t new_count = pages_size + 1;

    #ifdef ARENA_POOL_CPP
    if(arena)
      new_buffer = arena->allocate_size<pool_item<T>>(size);
    else
    #endif
      new_buffer = static_cast<pool_item<T>*>(malloc(sizeof(pool_item<T>) * size));

    if(!new_buffer) return false;

    pool_page<T>* new_list = nullptr;

    #ifdef ARENA_POOL_CPP
    if(arena)
      new_list = arena->allocate_size<pool_page<T>>(new_count);
    else if(pages_size)
      new_list = static_cast<pool_page<T>*>(realloc(pages, sizeof(pool_page<T>) * new_count));
    else
      new_list = static_cast<pool_page<T>*>(malloc(sizeof(pool_page<T>) * new_count));
    #else
    if(pages_size)
      new_list = static_cast<pool_page<T>*>(realloc(pages, sizeof(pool_page<T>) * new_count));
    else
      new_list = static_cast<pool_page<T>*>(malloc(sizeof(pool_page<T>) * new_count));
    #endif

    if(!new_list) {
      #ifdef ARENA_POOL_CPP
      if(!arena)
      #endif
        free(new_buffer);

      return false;
    }

    #ifdef ARENA_POOL_CPP
    if(arena && pages_size)
      memcpy(new_list, pages, sizeof(pool_page<T>) * pages_size);
    #endif

    new_list[new_count - 1] = { new_buffer, size };
    pages = new_list;
    pages_size = new_count;

    for(size_t i = 1; i < size; i++) {
      new_buffer[i - 1].next = &(new_buffer[i]);
    }
    
    new_buffer[size - 1].next = free_ptr;
    free_ptr = &(new_buffer[0]);

    return true;
  }
};

template <typename T, size_t S>
class pool_fixed {
private:
  pool_page_fixed<T, S> pages[1];  
  size_t pages_size = 1;

public:
  T* allocate_raw() {
    if(!free_ptr) return nullptr;

    pool_item<T>* chunk = free_ptr;
    free_ptr = free_ptr->next;

    if(use_ptr) use_ptr->prev = chunk;

    chunk->next = use_ptr;
    chunk->prev = nullptr;
    use_ptr = chunk;
    _used++;

    return &chunk->value;
  }

  POOL_CLASS_COMMON

public:
  pool_fixed() {
    reset();
  }

  ~pool_fixed() {
    if(!std::is_trivially_destructible<T>::value) {
      while(use_ptr != nullptr) {
        use_ptr->value.~T();
        use_ptr = use_ptr->next;
      }
    }
  }
};

}
