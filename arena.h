/*
 * Package: arena_pool_cpp
 * Version: 0.1.6
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

#define ARENA_POOL_CPP 1

#ifndef SARRAY_STD_VECTORS_DISABLE 
  #include <vector>
#endif

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

template <typename T>
class ivector {
protected: 
  T* buffer;
  size_t buffer_size;
  size_t _used;

  // This virtual function is a performance problem, yes.
  // Will likely need to stop sharing this `ISArray` base
  // class between SArrayFixed and SArray, in order to get
  // rid of the virtual function.
  inline virtual void _maybe_grow(const size_t &count) {
  }

  inline void maybe_grow(const size_t &count) {
    _maybe_grow(count);
  }

public:
  class iterator {
    T* it;
    bool _reverse;
  public:
    explicit iterator(T* begin, bool reverse = false):
      it(begin), _reverse(reverse) { }

    T& operator*() const { return *it; }

    bool operator!=(const iterator& other) const { return it != other.it; }

    iterator& operator+(size_t count) {
      if(_reverse) it -= count;
      else it += count;

      return *this;
    }

    iterator& operator++() {
      if(_reverse) it--;
      else it++;

      return *this;
    }
  };

  ivector() :
    buffer(nullptr),
    buffer_size(0),
    _used(0) { }

  ivector& operator=(const std::initializer_list<T> list) {
    maybe_grow(list.size());

    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : list)
      if(!push(item)) break;

    return *this;
  }

  ivector& operator=(const std::initializer_list<T>* list) {
    maybe_grow(list->size());

    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : *list)
      if(!push(item)) break;

    return *this;
  }

  ivector& operator=(const ivector& other) {
    maybe_grow(other.used());

    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : other)
      if(!push(item)) break;

    return *this;
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  ivector& operator=(const std::vector<T>& other) {
    maybe_grow(other.size());

    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : other)
      if(!push(item)) break;

    return *this;
  }
  #endif

  T& operator[](const size_t i) {
    return buffer[i];
  }

  const T& operator[](const size_t i) const {
    return buffer[i];
  }

  iterator begin() const {
    return iterator(buffer);
  }

  iterator rbegin() const {
    return iterator(buffer + _used - 1, true);
  }

  const iterator end() const {
    return iterator(buffer + _used);
  }

  const iterator rend() const {
    return iterator(buffer - 1);
  }

  T* first() {
    return _used ? buffer : nullptr;
  }

  const T* first() const {
    return _used ? buffer : nullptr;
  }

  T* last() {
    return _used ? buffer + _used - 1 : nullptr;
  }

  const T* last() const {
    return _used ? buffer + _used - 1 : nullptr;
  }

  T& at(const size_t pos) {
    return buffer[pos];
  }

  const T& at(const size_t pos) const {
    return buffer[pos];
  }

  template <typename U>
  T& replace(const size_t pos, U &&item) {
    buffer[pos] = item;

    return buffer[pos];
  }

  template <typename U>
  T& replace(iterator pos, U &&item) {
    return replace(&*pos - buffer, item);
  }

  template <typename... Args>
  T& replace_new(const size_t pos, Args&&... args) {
    buffer[pos] = T(std::forward<Args>(args)...);

    return buffer[pos];
  }

  template <typename... Args>
  T* replace_new(iterator pos, Args&&... args) {
    return replace_new(&*pos - buffer, args...);
  }

private:
  void insert_make_room_for_count(const size_t &position, const size_t& count) {
    if(position == _used) return;

    if(std::is_trivially_copyable<T>::value) {
      memmove(
        &buffer[position + count],
        &buffer[position],
        sizeof(T) * (_used - position)
      );
    }

    if(!std::is_trivially_destructible<T>::value ||
      !std::is_trivially_copyable<T>::value
    ) {
      for(size_t i = _used - 1 + count; i >= (position + count); i--) {
        if(!std::is_trivially_copyable<T>::value)
          new (&buffer[i]) T(std::move(buffer[i - count]));

        if(!std::is_trivially_destructible<T>::value)
          buffer[i - count].~T();
      }
    }
  }

public:
  template <typename U>
  T* insert(size_t position, const size_t count, U &&item) {
    maybe_grow(count);

    if(!count || _used + count > buffer_size || position > _used || position == buffer_size)
      return nullptr;

    insert_make_room_for_count(position, count);
       
    for(size_t i = 0; i < count; i++) {
      if(std::is_trivially_copyable<T>::value)
        memcpy(&buffer[position + i], &item, sizeof(T));
      else
        new (&buffer[position + i]) T(std::forward<U>(item)); 
    }

    _used += count;

    return &buffer[position];
  }

  template <typename U>
  T* insert(size_t position, U &&item) {
    return insert(position, 1, item);
  }

  template <typename U>
  T* insert(iterator pos, const size_t count, U &&item) {
    return insert(&*pos - buffer, count, item);
  }

  template <typename U>
  T* insert(iterator pos, U &&item) {
    return insert(pos, 1, item);
  }

  T* insert(size_t position, const std::initializer_list<T> list) {
    size_t count = list.size();
    maybe_grow(count);

    if(!count || _used + count > buffer_size || position > _used || position == buffer_size)
      return nullptr;

    insert_make_room_for_count(position, count);

    if(std::is_trivial<T>::value) {
      memcpy(&buffer[position], list.begin(), sizeof(T) * count);
    } else {
      size_t i = 0;
      for(const auto &it : list) {
        new (&buffer[position + i]) T(std::move(it)); 

        i++;
      }
    }

    _used += count;

    return &buffer[position];
  }

  T* insert(iterator pos, const std::initializer_list<T> list) {
    return insert(&*pos - buffer, list);
  }

  template <typename... Args>
  T* insert_new(size_t position, Args&&... args) {
    maybe_grow(1);

    if(_used == buffer_size || position > _used || position == buffer_size)
      return nullptr;

    insert_make_room_for_count(position, 1);

    new (&buffer[position]) T(std::forward<Args>(args)...);

    _used++;

    return &buffer[position];
  }

  template <typename... Args>
  T* insert_new(iterator pos, Args&&... args) {
    return insert_new(&*pos - buffer, args...);
  }

  template <typename U>
  T* push(U &&item) {
    maybe_grow(1);

    if(_used == buffer_size) return nullptr;

    if(std::is_trivially_copyable<T>::value) {
      memcpy(buffer + _used, &item, sizeof(T));
    } else {
      new (buffer + _used) T(std::forward<U>(item));
    }

    _used++;

    return buffer + _used - 1;
  }

  template <typename... Args>
  T* push_new(Args&&... args) {
    maybe_grow(1);

    if(_used == buffer_size) return nullptr;

    new (buffer + _used) T(std::forward<Args>(args)...);

    _used++;

    return buffer + _used - 1;
  }

  void pop() {
    if(_used) erase(_used - 1);
  }

  void erase_ptr(const T *item) {
    erase(((size_t)item - (size_t)buffer) / sizeof(T));
  }

  void erase(const size_t pos) {
    if(!buffer_size || pos <  0 || pos >= _used) return;

    if(!std::is_trivially_destructible<T>::value)
      buffer[pos].~T();

    if(pos < _used - 1) {
      if(std::is_trivially_copyable<T>::value && std::is_trivially_destructible<T>::value) {
        memmove(buffer + pos, buffer + pos + 1, sizeof(T) * (_used - 1 - pos));
      } else {
        for(size_t i = pos + 1; i < _used; i++) {
          if(std::is_trivially_copyable<T>::value)
            memcpy(&(buffer[pos - 1]), &(buffer[pos]), sizeof(T));
          else
            new (&buffer[i - 1]) T(std::move(buffer[i]));

          if(!std::is_trivially_destructible<T>::value)
            buffer[i].~T();
        }
      }
    }

    _used--;
  }

  void reset() {
    for(size_t i = 0; i < _used; i++)
      if(!std::is_trivially_destructible<T>::value)
        buffer[i].~T();

    _used = 0;
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

  arena* arena() const {
    return nullptr;
  }
};

template <typename T, bool Trivial>
class vector_fixed_mixin : public ivector<T> {
  ~vector_fixed_mixin() {
    for(size_t i = 0; i < this->_used; i++)
      this->buffer[i].~T();
  }
};

template <typename T>
class vector_fixed_mixin<T, true> {
};

template <typename T, size_t N>
class vector_fixed final : public ivector<T>, public vector_fixed_mixin<T, std::is_trivially_destructible<T>::value> {
  char static_buffer[sizeof(T) * N];

public:
  using ivector<T>::operator=;

  vector_fixed() : ivector<T>() {
    this->buffer = reinterpret_cast<T*>(static_buffer);
    this->buffer_size = N;
  }

  vector_fixed(const std::initializer_list<T> list) : vector_fixed() {
    this->operator=(&list);
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  vector_fixed(const std::vector<T>& other) : vector_fixed() {
    this->operator=(other);
  }
  #endif

  vector_fixed(const ivector<T>& other) : vector_fixed() {
    this->operator=(other);
  }
};

template <typename T>
class vector final : public ivector<T> {
  arena* _arena;

protected:
  void moved_reset() {
    this->buffer = nullptr;
    this->buffer_size = 0;
    this->_used = 0;
  }

  inline void _maybe_grow(const size_t &count) override {
    if(!this->buffer_size) {
      this->init(count);

      return;
    }

    if((this->buffer_size - this->_used) >= count) return;

    size_t new_size = this->buffer_size * 2;
    size_t min = this->_used + count; 

    if(new_size < min) new_size = min;

    this->resize(new_size);
  }

public:
  using ivector<T>::operator=;

  vector(const size_t size = 0) : ivector<T>(), _arena(nullptr) {
    init(size);
  }

  vector(const std::initializer_list<T> list) :
    vector(list.size())
  {
    this->operator=(&list);
  }

  vector(const size_t size, const std::initializer_list<T> list) :
    vector(size)
  {
    this->operator=(&list);
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  vector(const std::vector<T>& other) :
    vector(other.size())
  {
    this->operator=(other);
  }

  vector(const size_t size, const std::vector<T>& other) :
    vector(size)
  {
    this->operator=(other);
  }
  #endif

  // Move constructor.
  // (Can only move SArray, not SArrayFixed)
  vector(vector<T>&& other) : vector() {
    if(other.arena()) _arena = other.arena();

    this->buffer = other.buffer;
    this->_used = other._used;
    this->buffer_size = other.buffer_size;

    other.moved_reset();
  }

  vector(const vector<T>& other) :
    vector(other._arena, other.buffer_size)
  {
    this->operator=(other);
  }

  vector(const ivector<T>& other) :
    vector(other.size())
  {
    this->operator=(other);
  }

  vector(const size_t size, const ivector<T>& other) :
    vector(size)
  {
    this->operator=(other);
  }

  vector(arena* __arena, const size_t size) : ivector<T>(), _arena(nullptr) {
    if(__arena) init(*__arena, size);
    else init(size);
  }

  vector(arena &__arena, const size_t size) : ivector<T>(), _arena(nullptr) {
    init(__arena, size);
  }

  vector(arena &__arena, const size_t size, const std::initializer_list<T> list) :
    vector(__arena, size) 
  {
    this->operator=(&list);
  }

  vector(arena &__arena, const size_t size, const std::vector<T>& other) :
    vector(__arena, size) 
  {
    this->operator=(other);
  }

  vector(arena &__arena, const size_t size, const ivector<T>& other) :
    vector(__arena, size) 
  {
    this->operator=(other);
  }

  ~vector() {
    if(!std::is_trivially_destructible<T>::value) {
      for(size_t i = 0; i < this->_used; i++)
        this->buffer[i].~T();
    }

    if(!_arena && this->buffer_size)
      free(this->buffer);
  }

  vector& operator=(const vector& other) = default;

  void init(const size_t size) {
    if(!size || _arena || this->buffer_size) return;

    this->buffer = static_cast<T*>(malloc(sizeof(T) * size));

    if(this->buffer) this->buffer_size = size;
  }

  void init(arena &__arena, const size_t size) {
    if(!size || _arena || this->buffer_size) return;

    auto* new_buffer = __arena.allocate_size<T>(size);

    if(new_buffer) {
      _arena = &__arena;
      this->buffer = new_buffer;
      this->buffer_size = size;
    }
  }

  bool resize(const size_t size) {
    if(size == this->buffer_size) return true;

    T* new_buffer = nullptr;
    size_t copy_size = size > this->buffer_size ? this->buffer_size : size;

    if(_arena) {
      if(size < this->buffer_size) return false;

      new_buffer = _arena->allocate_size<T>(size);

      if(new_buffer) {
        if(std::is_trivially_copyable<T>::value)
          memcpy(new_buffer, this->buffer, sizeof(T) * copy_size);

        if(!std::is_trivially_destructible<T>::value ||
            !std::is_trivially_copyable<T>::value
        ) {
          for(size_t i = 0; i < this->_used; i++) {
            if(!std::is_trivially_copyable<T>::value &&
              i < copy_size
            ) new (&new_buffer[i]) T(std::move(this->buffer[i]));

            if(!std::is_trivially_destructible<T>::value)
              this->buffer[i].~T();
          }
        }
      }
    } else {
      if(std::is_trivially_copyable<T>::value)
        new_buffer = static_cast<T*>(
          realloc(this->buffer, sizeof(T) * size)
        );
      else
        new_buffer = static_cast<T*>(malloc(sizeof(T) * size));

      if(new_buffer && (!std::is_trivially_destructible<T>::value ||
        !std::is_trivially_copyable<T>::value)
      ) {
        for(size_t i = 0; i < this->_used; i++) {
            if(!std::is_trivially_copyable<T>::value && i < copy_size)
              new (&new_buffer[i]) T(std::move(this->buffer[i]));

            if(!std::is_trivially_destructible<T>::value)
              this->buffer[i].~T();
        }

        if(!std::is_trivially_copyable<T>::value)
          free(this->buffer);
      }
    }

    if(new_buffer) {
      this->buffer = new_buffer;
      this->buffer_size = size;

      if(this->_used > size) {
        this->_used = size;
      }

      return true;
    }

    return false;
  }

  bool shrink_to_fit() {
    if(this->_used < this->buffer_size && this->buffer_size > 1) {
      return resize(this->_used ? this->_used : 1);
    }

    return 0;
  }

  arena* arena() const {
    return _arena;
  }
};

}
