/*
 * Package: arena_pool_cpp
 * Version: 0.2.0
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

class arena {
private:
  arena* parent;
  char* buffer;
  size_t total_size;
  size_t offset;

public:
  arena(const size_t size) :
    parent(nullptr),
    buffer(static_cast<char*>(malloc(size))),
    total_size(size),
    offset(0) {}

  arena(arena &parent, const size_t size) :
    parent(&parent),
    buffer(static_cast<char*>(parent.allocate_raw(size))),
    total_size(size),
    offset(0) {}

  ~arena() {
    if(!parent) free(buffer);
  }

  template <typename T, typename... Args>
  T* allocate_new(Args&&... args) {
    T* new_item = allocate_size<T>();

    if(!new_item) return nullptr;

    return new (new_item) T(std::forward<Args>(args)...);
  }

  template <typename T>
  T* allocate_size(const size_t count = 1) {
    if(!buffer) return nullptr;

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
    uintptr_t current_addr = reinterpret_cast<uintptr_t>(buffer) + offset;
    size_t padding = (alignment - (current_addr % alignment)) % alignment;

    size_t new_size = padding + size;

    if (offset + new_size > total_size) {
      return nullptr;
    }

    char* new_allocation = buffer + offset + padding;
    offset += new_size;

    return static_cast<void*>(new_allocation);
  }

  bool resize(const size_t size) {
    char* new_buffer = static_cast<char*>(
        parent ? parent->allocate_raw(size) :
        malloc(size)
    );

    if(!new_buffer) return false;

    if(!parent) free(buffer);

    reset();

    buffer = new_buffer;
    total_size = size;

    return true;
  }

  void reset() {
    offset = 0;
  }

  size_t size() const {
    return total_size;
  }

  size_t used() const {
    return offset;
  }
};

template <typename T>
struct pool_item {
  pool_item* next; 
  pool_item* prev; 
  T value;
};


template <typename T>
struct pool_buffer {
  pool_item<T>* buffer;
  size_t size;
};

template <typename T>
class pool {
private:
  arena* arena;
  pool_buffer<T>* buffers;
  size_t buffers_size;
  pool_item<T>* free_ptr;
  pool_item<T>* use_ptr;
  size_t _used;

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

public:
  pool(apc::arena &_arena, const size_t pool_size) :
    arena(&_arena),
    buffers(arena->allocate_size<pool_buffer<T>>()),
    buffers_size(1),
    free_ptr(nullptr),
    use_ptr(nullptr),
    _used(0)
  {
    buffers[0] = { arena->allocate_size<pool_item<T>>(pool_size), pool_size };
    reset();
  }

  pool(const size_t pool_size) :
    arena(nullptr),
    buffers(static_cast<pool_buffer<T>*>(malloc(sizeof(pool_buffer<T>)))),
    buffers_size(1),
    free_ptr(nullptr),
    use_ptr(nullptr),
    _used(0)
  {
    buffers[0] = {
      static_cast<pool_item<T>*>(malloc(sizeof(pool_item<T>) * pool_size)),
      pool_size
    };

    reset();
  }

  ~pool() {
    if(!std::is_trivially_destructible<T>::value) {
      while(use_ptr) {
        use_ptr->value.~T();
        use_ptr = use_ptr->next;
      }
    }

    if(!arena) {
      for(size_t i = 0; i < buffers_size; i++) {
        free(buffers[i].buffer);
      }

      free(buffers);
    }
  }

  void reset() {
    if(!std::is_trivially_destructible<T>::value) {
      while(use_ptr) {
        use_ptr->value.~T();
        use_ptr = use_ptr->next;
      }
    }

    pool_item<T>* previous = nullptr;
    for(size_t i = 0; i < buffers_size; i++) {
      for(size_t z = 0; z < buffers[i].size; z++) {
        if(previous) previous->next = &(buffers[i].buffer[z]);

        previous = &(buffers[i].buffer[z]);
        previous->prev = nullptr;
        previous->next = nullptr;
      }
    }

    previous->next = nullptr;
    free_ptr = &(buffers[0].buffer[0]);

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
    size_t new_count = buffers_size + 1;

    if(arena)
      new_buffer = arena->allocate_size<pool_item<T>>(size);
    else
      new_buffer = static_cast<pool_item<T>*>(malloc(sizeof(pool_item<T>) * size));

    if(!new_buffer) return false;

    pool_buffer<T>* new_list = nullptr;

    if(arena)
      new_list = arena->allocate_size<pool_buffer<T>>(new_count);
    else
      new_list = static_cast<pool_buffer<T>*>(realloc(buffers, sizeof(pool_buffer<T>) * new_count));

    if(!new_list) {
      free(new_buffer);
      return false;
    }

    if(arena) {
      for(size_t i = 0; i < buffers_size; i++) {
        new_list[i] = buffers[i];
      }
    }

    new_list[new_count - 1] = { new_buffer, size };
    buffers = new_list;
    buffers_size = new_count;

    for(size_t i = 1; i < size; i++) {
      new_buffer[i - 1].next = &(new_buffer[i]);
    }
    
    new_buffer[size - 1].next = free_ptr;
    free_ptr = &(new_buffer[0]);

    return true;
  }

  size_t size() const {
    size_t total_size = 0;

    for(size_t i = 0; i < buffers_size; i++) {
      total_size += buffers[i].size;
    }

    return total_size;
  }

  size_t used() const {
    return _used;
  }
};

}
