/*
 * Package: arena_pool_cpp
 * Version: 0.0.1
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast (zero-overhead) memory allocator with
 *              an arena/bump-allocator, and a pool-allocator.
 *              Single header c++11 library.
 */
#pragma once

#include <utility>
#include <cstdlib>
#include <cstddef>

class Arena {
private:
  char* buffer;
  size_t total_size;
  size_t offset;
  bool is_child;

public:
  Arena(size_t size) :
    buffer(static_cast<char*>(malloc(size))),
    total_size(size),
    offset(0),
    is_child(false) {}

  Arena(Arena &parent, size_t size) :
    buffer(static_cast<char*>(parent.allocate_raw(size))),
    total_size(size),
    offset(0),
    is_child(true) {}

  ~Arena() {
    if(!is_child) free(buffer);
  }

  template <typename T, typename... Args>
  T* allocate_new(Args&&... args) {
    return new (allocate<T>()) T(std::forward<Args>(args)...);
  }

  template <typename T>
  T* allocate(size_t count = 1) {
    if(!buffer) return nullptr;

    size_t alignment = alignof(T);
    size_t size = sizeof(T) * count;

    return static_cast<T*>(allocate_raw(size, alignment));
  }

  void* allocate_raw(size_t size,
    size_t alignment = alignof(std::max_align_t)
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

  void reset() {
    offset = 0;
  }

  size_t size() {
    return total_size;
  }

  size_t used() {
    return offset;
  }
};

template <typename T>
struct PoolItem {
  PoolItem* next; 
  T value;
};


template <typename T>
struct PoolBuffer {
  PoolItem<T>* buffer;
  size_t size;
};

template <typename T>
class Pool {
private:
  Arena* arena;
  PoolBuffer<T>* buffers;
  size_t buffers_size;
  PoolItem<T>* free_ptr;
  size_t _used;

public:
  Pool(Arena &_arena, size_t pool_size) :
    arena(&_arena),
    buffers(arena->allocate<PoolBuffer<T>>()),
    buffers_size(1),
    free_ptr(nullptr),
    _used(0)
  {
    buffers[0] = { arena->allocate<PoolItem<T>>(pool_size), pool_size };
    reset();
  }

  Pool(size_t pool_size) :
    arena(nullptr),
    buffers(static_cast<PoolBuffer<T>*>(malloc(sizeof(PoolBuffer<T>)))),
    buffers_size(1),
    free_ptr(nullptr),
    _used(0)
  {
    buffers[0] = {
      static_cast<PoolItem<T>*>(malloc(sizeof(PoolItem<T>) * pool_size)),
      pool_size
    };

    reset();
  }

  ~Pool() {
    if(!arena) {
      for(size_t i = 0; i < buffers_size; i++) {
        free(buffers[i].buffer);
      }

      free(buffers);
    }
  }

  void reset() {
    PoolItem<T>* previous = nullptr;
    for(size_t i = 0; i < buffers_size; i++) {
      for(size_t z = 0; z < buffers[i].size; z++) {
        if(previous) previous->next = &(buffers[i].buffer[z]);

        previous = &(buffers[i].buffer[z]);
      }
    }

    previous->next = nullptr;
    free_ptr = &(buffers[0].buffer[0]);

    _used = 0;
  }

  template <typename... Args>
  T* allocate_new(Args&&... args) {
    return new (allocate()) T(std::forward<Args>(args)...);
  }

  T* allocate() {
    if(!free_ptr) return nullptr;

    PoolItem<T>* chunk = free_ptr;
    free_ptr = free_ptr->next;
    _used++;

    return &chunk->value;
  }

  void deallocate(T* ptr) {
    if(ptr == nullptr) return;

    ptr->~T();

    // Use offsetof to safely get the address of the containing PoolItem.
    // We subtract the offset of the 'value' member from the 'ptr'.
    PoolItem<T>* item = reinterpret_cast<PoolItem<T>*>(
      reinterpret_cast<T*>(ptr) - offsetof(PoolItem<T>, value)
    );

    item->next = free_ptr;
    free_ptr = item;
    _used--;
  }

  bool grow(size_t size) {
    PoolItem<T>* new_buffer = nullptr;
    size_t new_count = buffers_size + 1;

    if(arena)
      new_buffer = arena->allocate<PoolItem<T>>(size);
    else
      new_buffer = static_cast<PoolItem<T>*>(malloc(sizeof(PoolItem<T>) * size));

    if(!new_buffer) return false;

    PoolBuffer<T>* new_list = nullptr;

    if(arena)
      new_list = arena->allocate<PoolBuffer<T>>(new_count);
    else
      new_list = static_cast<PoolBuffer<T>*>(malloc(sizeof(PoolBuffer<T>) * new_count));

    if(!new_list) {
      free(new_buffer);
      return false;
    }

    for(size_t i = 0; i < buffers_size; i++) {
      new_list[i] = buffers[i];
    }

    if(!arena) free(buffers);

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

  size_t size() {
    size_t total_size = 0;

    for(size_t i = 0; i < buffers_size; i++) {
      total_size += buffers[i].size;
    }

    return total_size;
  }

  size_t used() {
    return _used;
  }
};
