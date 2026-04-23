/*
 * Package: arena_pool_cpp
 * Version: 0.2.2
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

#include <utility>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <new>

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
    size_t alignment = alignof(T);
    size_t size = sizeof(T) * count;

    return static_cast<T*>(allocate_raw(size, alignment));
  }

  template <typename T>
  T* allocate(T&& item) {
    T* new_item = allocate_size<T>();

    if(!new_item) return nullptr;

    if(std::is_trivially_copyable<T>::value)
      memcpy(new_item, &item, sizeof(T));
    else
      new (new_item) T(item);

    return new_item;
  }

  void* allocate_raw(const size_t size,
    const size_t alignment = alignof(std::max_align_t)
  ) {
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

template <typename T>
class pool {
private:
  arena* arena = nullptr;
  pool_page<T>* pages = nullptr;
  size_t pages_size = 0;
  pool_item<T>* free_ptr = nullptr;
  pool_item<T>* use_ptr = nullptr;
  size_t _used = 0;

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

public:
  pool(apc::arena &_arena, const size_t pool_size = 0) : arena(&_arena) {
    if(pool_size) grow(pool_size);
  }

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

    if(!arena) {
      for(size_t i = 0; i < pages_size; i++) {
        free(pages[i].buffer);
      }

      free(pages);
    }
  }

  void reset() {
    if(!pages_size) return;

    if(!std::is_trivially_destructible<T>::value) {
      while(use_ptr) {
        use_ptr->value.~T();
        use_ptr = use_ptr->next;
      }
    }

    pool_item<T>* previous = nullptr;
    for(size_t i = 0; i < pages_size; i++) {
      for(size_t z = 0; z < pages[i].size; z++) {
        if(previous) previous->next = &(pages[i].buffer[z]);

        previous = &(pages[i].buffer[z]);
        previous->prev = nullptr;
        previous->next = nullptr;
      }
    }

    previous->next = nullptr;
    free_ptr = &(pages[0].buffer[0]);

    _used = 0;
  }

  template <typename... Args>
  T* allocate_new(Args&&... args) {
    T* new_item = allocate_raw();

    if(!new_item) return nullptr;

    return new (new_item) T(std::forward<Args>(args)...);
  }

  template <typename U>
  T* allocate(U &&item) {
    T* new_item = allocate_raw();

    if(!new_item) return nullptr;

    if(std::is_trivially_copyable<T>::value)
      memcpy(new_item, &item, sizeof(T));
    else
      new (new_item) T(std::forward<U>(item));

    return new_item;
  }

  void deallocate(T* ptr) {
    if(ptr == nullptr) return;

    // Use offsetof to safely get the address of the containing PoolItem.
    // We subtract the offset of the 'value' member from the 'ptr'.
    pool_item<T>* item = reinterpret_cast<pool_item<T>*>(
      reinterpret_cast<char*>(ptr) - offsetof(pool_item<T>, value)
    );

    // ptr->prev is only set if allocated, otherwise if an item is free
    // it is nullptr.
    if(use_ptr != item && item->prev == nullptr) return;

    if(!std::is_trivially_destructible<T>::value) item->value.~T();

    if(item->prev) item->prev->next = item->next;
    if(item->next) item->next->prev = item->prev;
    if(use_ptr == item) use_ptr = item->next;

    item->next = free_ptr;
    item->prev = nullptr;
    free_ptr = item;

    _used--;
  }

  bool grow(const size_t size) {
    pool_item<T>* new_buffer = nullptr;
    size_t new_count = pages_size + 1;

    if(arena)
      new_buffer = arena->allocate_size<pool_item<T>>(size);
    else
      new_buffer = static_cast<pool_item<T>*>(malloc(sizeof(pool_item<T>) * size));

    if(!new_buffer) return false;

    pool_page<T>* new_list = nullptr;

    if(arena)
      new_list = arena->allocate_size<pool_page<T>>(new_count);
    else if(pages_size)
      new_list = static_cast<pool_page<T>*>(realloc(pages, sizeof(pool_page<T>) * new_count));
    else
      new_list = static_cast<pool_page<T>*>(malloc(sizeof(pool_page<T>) * new_count));

    if(!new_list) {
      if(!arena) free(new_buffer);

      return false;
    }

    if(arena && pages_size)
      memcpy(new_list, pages, sizeof(pool_page<T>) * pages_size);

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

  size_t size() const {
    size_t total_size = 0;

    for(size_t i = 0; i < pages_size; i++) {
      total_size += pages[i].size;
    }

    return total_size;
  }

  size_t used() const {
    return _used;
  }
};

}
