/*
 * Package: arena_pool_cpp
 * Version: 0.0.2
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast (zero-overhead) memory allocator with
 *              an arena/bump-allocator, a pool-allocator, and a
 *              vector-like array-allocator.
 *              Single header c++11 library.
 */
#pragma once

#include <utility>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <new>

class Arena {
private:
  Arena* parent;
  char* buffer;
  size_t total_size;
  size_t offset;

public:
  Arena(size_t size) :
    parent(nullptr),
    buffer(static_cast<char*>(malloc(size))),
    total_size(size),
    offset(0) {}

  Arena(Arena &parent, size_t size) :
    parent(&parent),
    buffer(static_cast<char*>(parent.allocate_raw(size))),
    total_size(size),
    offset(0) {}

  ~Arena() {
    if(!parent) free(buffer);
  }

  template <typename T, typename... Args>
  T* allocate_new(Args&&... args) {
    T* new_item = allocate<T>();

    if(!new_item) return nullptr;

    return new (new_item) T(std::forward<Args>(args)...);
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

  bool resize(size_t size) {
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
    T* new_item = allocate();

    if(!new_item) return nullptr;

    return new (new_item) T(std::forward<Args>(args)...);
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
      reinterpret_cast<char*>(ptr) - offsetof(PoolItem<T>, value)
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
      new_list = static_cast<PoolBuffer<T>*>(realloc(buffers, sizeof(PoolBuffer<T>) * new_count));

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

template <typename T>
class SArray {
private: 
  Arena* arena;
  T* buffer;
  bool* active;
  size_t buffer_size;
  size_t _used;
  size_t _last;

  void maybe_set_last(size_t pos) {
    if(_last != pos) return;

    _last = 0;

    if(!_used) return;

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) _last = i + 1;
    }
  }

public:
  class iterator {
    T* ptr;
    bool* active;
    T* end;
    size_t i;
  public:
    explicit iterator(T* _ptr, bool* a, T* e, bool empty) :
      ptr(_ptr),
      active(a),
      end(e),
      i(0)
    {
      if(!empty) {
        while((ptr + i) != end && !active[i])
          i++;
      }
    }
    T& operator*() const { return ptr[i]; }
    iterator& operator++() {
      i++;

      while((ptr + i) != end && !active[i]) {
        i++;
      }

      return *this;
    }
    bool operator!=(const iterator& other) const { return (ptr + i) != other.ptr; }
  };

  SArray(size_t size) :
    arena(nullptr),
    buffer(static_cast<T*>(malloc(sizeof(T) * size))),
    active(static_cast<bool*>(malloc(sizeof(bool) * size))),
    buffer_size(size),
    _used(0),
    _last(0)
  {
    if(!buffer || !active) buffer_size = 0;
    if(!buffer && active) free(active);
    if(!active && buffer) free(buffer);
  }

  SArray(Arena &_arena, size_t size) :
    arena(&_arena),
    buffer(arena->allocate<T>(size)),
    active(arena->allocate<bool>(size)),
    buffer_size(size),
    _used(0),
    _last(0)
  {
    if(!buffer || !active) buffer_size = 0;
    if(!buffer && active) free(active);
    if(!active && buffer) free(buffer);
  }

  ~SArray() {
    if(!arena && buffer_size) {
      free(buffer);
      free(active);
    }
  }

  T* operator[](size_t i) {
    if(!_used || i >= buffer_size ||  !active[i]) return nullptr;
    return &(buffer[i]);
  }

  const T* operator[](size_t i) const {
    if(!_used || i >= buffer_size ||  !active[i]) return nullptr;
    return &(buffer[i]);
  }

  iterator begin() {
    return iterator(buffer, active, &(buffer[_last]), _used == 0);
  }

  const iterator begin() const {
    return iterator(buffer, active, &(buffer[_last]), _used == 0);
  }

  iterator end() {
    return iterator(&(buffer[_last]), active, &(buffer[_last]), true);
  }

  const iterator end() const {
    return iterator(&(buffer[_last]), active, &(buffer[_last]), true);
  }

  T* first() {
    if(_used) {
      for(size_t i = 0; i < buffer_size; i++) {
        if(active[i]) return &(buffer[i]);
      }
    }

    return nullptr;
  }

  const T* first() const {
    if(_used) {
      for(size_t i = 0; i < buffer_size; i++) {
        if(active[i]) return &(buffer[i]);
      }
    }

    return nullptr;
  }

  T* last() {
    if(!_used) return nullptr;

    return &(buffer[_last - 1]);
  }

  const T* last() const {
    if(!_used) return nullptr;

    return &(buffer[_last - 1]);
  }

  T* at(size_t pos) {
    if(!_used || pos >= buffer_size ||  !active[pos]) return nullptr;

    return &(buffer[pos]);
  }

  const T* at(size_t pos) const {
    if(!_used || pos >= buffer_size ||  !active[pos]) return nullptr;

    return &(buffer[pos]);
  }

  template <typename U>
  T* fill(U &&item) {
    if(_used == buffer_size) return nullptr;

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) continue;

      if(std::is_trivially_copyable<T>::value) {
        memcpy(&(buffer[i]), &item, sizeof(T));
      } else {
        buffer[i] = std::forward<U>(item);
      }

      active[i] = true;
      _used++;

      if(i == _last) _last++;

      return &(buffer[i]);
    }

    return nullptr;
  }

  template <typename... Args>
  T* fill_new(Args&&... args) {
    if(_used == buffer_size) return nullptr;

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) continue;

      new (&(buffer[i])) T(std::forward<Args>(args)...);

      active[i] = true;
      _used++;

      if(i == _last) _last++;

      return &(buffer[i]);
    }

    return nullptr;
  }

  template <typename U>
  T* push(U &&item) {
    if(_used == buffer_size) return nullptr;

    if(std::is_trivially_copyable<T>::value) {
      memcpy(&(buffer[_last]), &item, sizeof(T));
    } else {
      buffer[_last] = std::forward<U>(item);
    }

    active[_last] = true;
    _used++;
    _last++;

    return &(buffer[_last - 1]);
  }

  template <typename... Args>
  T* push_new(Args&&... args) {
    if(_used == buffer_size) return nullptr;

    new (&(buffer[_last])) T(std::forward<Args>(args)...);
    active[_last] = true;
    _used++;
    _last++;

    return &(buffer[_last - 1]);
  }

  void pop() {
    if(!_used) return;

    _used--;
    active[_last - 1] = false;

    maybe_set_last(_last);
  }

  void erase_ptr(T *item) {
    erase(((size_t)item - (size_t)buffer) / sizeof(T));
  }

  void erase(size_t pos) {
    if(pos <  0 || pos >= buffer_size || !active[pos]) return;

    active[pos] = false;
    _used--;

    maybe_set_last(pos);
  }

  void reset() {
    for(size_t i = 0; i < buffer_size; i++)
      active[i] = false;

    _used = 0;
    _last = 0;
  }

  void compact() {
    if(!_used) return;

    size_t target = -1;
    for(size_t i = 0; i < buffer_size; i++) {
      if(target ==  _used) {
        break;
      } else if(target == -1) { 
        if(!active[i]) target = i;
      } else if(active[i]) {
        if(std::is_trivially_copyable<T>())
          memcpy(&(buffer[target]), &(buffer[i]), sizeof(T));
        else
          buffer[target] = std::move(buffer[i]);

        active[i] = false;
        active[target] = true;
        target++;
      }
    }

    if(target != -1) _last = target;
  }

  bool resize(size_t size) {
    compact();

    if(size == buffer_size) return true;

    T* new_buffer = nullptr;
    bool *new_active = nullptr;

    size_t copy_size = size > buffer_size ? buffer_size : size;

    if(arena) {
      if(size < buffer_size) return false;

      new_buffer = arena->allocate<T>(size);

      if(new_buffer)
        new_active = arena->allocate<bool>(size);

      if(new_buffer && new_active) {
        if(std::is_trivially_copyable<T>()) {
          memcpy(new_buffer, buffer, sizeof(T) * copy_size);
        } else {
          for(size_t i = 0; i < copy_size; i++) {
            if(!active[i]) break;
            new_buffer[i] = std::move(buffer[i]);
          }
        }

        memcpy(new_active, active, sizeof(bool) * copy_size);
      } else {
        new_buffer = nullptr;
        new_active = nullptr;
      }
    } else {
      if(std::is_trivially_copyable<T>()) {
        new_buffer = static_cast<T*>(realloc(buffer, sizeof(T) * size));
      } else {
        new_buffer = static_cast<T*>(malloc(sizeof(T) * size));

        if(new_buffer) {
          for(size_t i = 0; i < copy_size; i++) {
              if(!active[i]) break;

              new_buffer = std::move(&(buffer[i]));
          }

          free(buffer);
        }
      }

      if(new_buffer) {
        new_active = static_cast<bool*>(realloc(active, sizeof(bool) * size));
      }
    }

    if(new_buffer) buffer = new_buffer;

    if(new_active) {
      if(size > buffer_size) {
        for(size_t i = copy_size; i < size; i++) {
          new_active[i] = false;
        }
      }

      if(_used > size) _used = size;

      active = new_active;
      buffer_size = size;

      return true;
    }

    return false;
  }

  size_t used() const {
    return _used;
  }

  size_t size() const {
    return buffer_size;
  }

  bool empty() const {
    return _used == 0;
  }
};
