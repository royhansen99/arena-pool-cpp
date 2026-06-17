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
#include <string.h>

#define ARENA_POOL_CPP 1

namespace apc {

struct arena_page {
  size_t size;
  size_t used;
  void* target; 
  char* buffer;
};

class arena {
private:
  arena* parent = nullptr;
  arena_page* pages = nullptr;
  size_t pages_size = 0;

public:
  arena(const size_t size) {
    if(size) grow(size);
  }

  arena(arena &parent, const size_t size) : parent(&parent) {
    if(size) grow(size);
  }

  ~arena() {
    if(!parent) {
      for(size_t i = 0; i < pages_size; i++)
        free(pages[i].target);
    }
  }

  template <typename T, typename... Args>
  T* allocate_new(Args&&... args) {
    T* new_item = allocate_size<T>();

    if(!new_item) return nullptr;

    return new (new_item) T(std::forward<Args>(args)...);
  }

  template <typename T>
  T* allocate_size(const size_t count = 1) {
    if(!count) return nullptr;

    size_t alignment = alignof(T);
    size_t size = sizeof(T) * count;

    return static_cast<T*>(allocate_raw(size, alignment));
  }

  template <typename T, bool FORCE_TRIVIAL_COPY = false>
  T* allocate(T& item) {
    T* new_item = allocate_size<T>();

    if(!new_item) return nullptr;

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
      memcpy(new_item, &item, sizeof(T));
    else
      new (new_item) T(item);

    return new_item;
  }

  template <typename T, bool FORCE_TRIVIAL_COPY = false>
  T* allocate(T&& item) {
    return allocate<T, FORCE_TRIVIAL_COPY>(item);
  }

  template <typename T, bool FORCE_TRIVIAL_COPY = false>
  T* allocate(T* item, size_t size) {
    T* new_item = allocate_size<T>(size);

    if(!new_item) return nullptr;

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
      memcpy(new_item, item, sizeof(T) * size);
    else {
      for(size_t i = 0; i < size; i++)
        new (&new_item[i]) T(item[i]);
    }

    return new_item;
  }

  void* allocate_raw(const size_t size,
    const size_t alignment = alignof(std::max_align_t)
  ) {
    if(!size) return nullptr;

    for(size_t i = 0; i <= pages_size; i++) {
      arena_page *page;
      char* buffer;

      if(i == pages_size) {
        const size_t grow_size = i == 0 ? size + alignment :
          pages[i - 1].size * 2;

        if(!grow(grow_size)) break; 
      }

      page = &pages[i];
      buffer = page->buffer + page->used;

      if(page->size - page->used < size) continue; 

      uintptr_t current_addr = reinterpret_cast<uintptr_t>(buffer);
      size_t padding = (alignment - (current_addr % alignment)) % alignment;
      size_t new_size = padding + size;

      if(page->size - page->used < new_size) continue;

      char* new_allocation = buffer + padding;
      page->used += new_size;

      return static_cast<void*>(new_allocation);
    }

    return nullptr;
  }

  bool resize(const size_t size) {
    if(!parent) {
      for(size_t i = 0; i < pages_size; i++)
        free(pages[i].target);
    }

    pages = nullptr;
    pages_size = 0;

    return grow(size);
  }

  bool grow(const size_t size) {
    const size_t new_size = (sizeof(arena_page) * (pages_size + 1)) + size;
    char* new_page;
    
    if(!parent)
      new_page = static_cast<char*>(malloc(new_size)); 
    else
      new_page = static_cast<char*>(parent->allocate_raw(new_size));

    if(!new_page) return false;

    if(pages_size)
      memcpy(new_page, pages, sizeof(arena_page) * pages_size);

    reinterpret_cast<arena_page*>(new_page)[pages_size] = {
      size, // size
      0, // used
      new_page, // target
      new_page + (sizeof(arena_page) * (pages_size + 1)), // buffer
    }; 

    pages = reinterpret_cast<arena_page*>(new_page);
    pages_size++;

    return true;
  }

  void reset() {
    for(size_t i = 0; i < pages_size; i++)
      pages[i].used = 0;
  }

  size_t size() const {
    size_t count = 0;

    for(size_t i = 0; i < pages_size; i++)
      count += pages[i].size;

    return count;
  }

  size_t used() const {
    size_t count = 0;

    for(size_t i = 0; i < pages_size; i++)
      count += pages[i].used;

    return count;
  }
};

}
