/*
 * Package: arena_pool_cpp
 * Version: 0.1.3
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

class Arena {
private:
  Arena* parent;
  char* buffer;
  size_t total_size;
  size_t offset;

public:
  Arena(const size_t size) :
    parent(nullptr),
    buffer(static_cast<char*>(malloc(size))),
    total_size(size),
    offset(0) {}

  Arena(Arena &parent, const size_t size) :
    parent(&parent),
    buffer(static_cast<char*>(parent.allocate_raw(size))),
    total_size(size),
    offset(0) {}

  ~Arena() {
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
struct PoolItem {
  PoolItem* next; 
  PoolItem* prev; 
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
  PoolItem<T>* use_ptr;
  size_t _used;

  T* allocate_raw() {
    if(!free_ptr) return nullptr;

    PoolItem<T>* chunk = free_ptr;
    free_ptr = free_ptr->next;

    if(use_ptr) use_ptr->prev = chunk;

    chunk->next = use_ptr;
    chunk->prev = nullptr;
    use_ptr = chunk;
    _used++;

    return &chunk->value;
  }

public:
  Pool(Arena &_arena, const size_t pool_size) :
    arena(&_arena),
    buffers(arena->allocate_size<PoolBuffer<T>>()),
    buffers_size(1),
    free_ptr(nullptr),
    use_ptr(nullptr),
    _used(0)
  {
    buffers[0] = { arena->allocate_size<PoolItem<T>>(pool_size), pool_size };
    reset();
  }

  Pool(const size_t pool_size) :
    arena(nullptr),
    buffers(static_cast<PoolBuffer<T>*>(malloc(sizeof(PoolBuffer<T>)))),
    buffers_size(1),
    free_ptr(nullptr),
    use_ptr(nullptr),
    _used(0)
  {
    buffers[0] = {
      static_cast<PoolItem<T>*>(malloc(sizeof(PoolItem<T>) * pool_size)),
      pool_size
    };

    reset();
  }

  ~Pool() {
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

    PoolItem<T>* previous = nullptr;
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
    PoolItem<T>* item = reinterpret_cast<PoolItem<T>*>(
      reinterpret_cast<char*>(ptr) - offsetof(PoolItem<T>, value)
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
    PoolItem<T>* new_buffer = nullptr;
    size_t new_count = buffers_size + 1;

    if(arena)
      new_buffer = arena->allocate_size<PoolItem<T>>(size);
    else
      new_buffer = static_cast<PoolItem<T>*>(malloc(sizeof(PoolItem<T>) * size));

    if(!new_buffer) return false;

    PoolBuffer<T>* new_list = nullptr;

    if(arena)
      new_list = arena->allocate_size<PoolBuffer<T>>(new_count);
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
class ISArray {
protected: 
  T* buffer;
  bool* active;
  size_t buffer_size;
  size_t _used;
  size_t _last;

  void maybe_set_last(const size_t pos) {
    if(_last != pos) return;

    _last = 0;

    if(!_used) return;

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) _last = i + 1;
    }
  }

public:
  class iterator {
    T* _begin;
    T* _end;
    bool* _active;
    T* it;
    bool _reverse;
  public:
    explicit iterator(T* begin, T* end, bool* active, bool reverse = false):
      _begin(begin), _end(end), _active(active), it(begin),
      _reverse(reverse) { }

    T& operator*() const { return *it; }

    bool operator!=(const iterator& other) const { return it != other.it; }

    iterator& operator+(size_t count) {
      if(_reverse) {
        it -= count;
        _active -= count;
      } else {
        it += count;
        _active += count;
      }

      return *this;
    }

    iterator& operator++() {
      if(_reverse) {
        it--;
        _active--;

        while(it != _end && !*_active) {
          it--;
          _active--;
        }
      } else {
        it++;
        _active++;

        while(it != _end && !*_active) {
          it++;
          _active++;
        }
      }

      return *this;
    }
  };

  ISArray() :
    buffer(nullptr),
    active(nullptr),
    buffer_size(0),
    _used(0),
    _last(0)
  { }

  ISArray& operator=(const std::initializer_list<T> list) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : list)
      if(!push(item)) break;

    return *this;
  }

  ISArray& operator=(const std::initializer_list<T>* list) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : *list)
      if(!push(item)) break;

    return *this;
  }

  ISArray& operator=(const ISArray& other) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : other)
      if(!push(item)) break;

    return *this;
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  ISArray& operator=(const std::vector<T>& other) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : other)
      if(!push(item)) break;

    return *this;
  }
  #endif

  T* operator[](const size_t i) {
    if(!_used || i >= buffer_size ||  !active[i]) return nullptr;
    return &(buffer[i]);
  }

  const T* operator[](const size_t i) const {
    if(!_used || i >= buffer_size ||  !active[i]) return nullptr;
    return &(buffer[i]);
  }

  iterator begin() const {
    T* first = buffer;
    bool* first_active = active;
    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) {
        first = &(buffer[i]);
        first_active = &(active[i]);
        break;
      }
    }

    return iterator(first, &(buffer[_last]), first_active);
  }

  iterator rbegin() const {
    T* last = buffer;

    if(!buffer_size)
      return iterator(last, last, active, true);

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) {
        last = &(buffer[i]);
        break;
      }
    }

    return iterator(&(buffer[_last - 1]), last - 1, &(active[_last - 1]), true);
  }

  const iterator end() const {
    if(!buffer_size)
      return iterator(buffer, buffer, active);

    return iterator(&(buffer[_last]), &(buffer[_last]), &(active[_last - 1]));
  }

  const iterator rend() const {
    T* last = buffer;

    if(!buffer_size)
      return iterator(buffer, buffer, active);

    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i]) {
        last = &(buffer[i]);
        break;
      }
    }

    return iterator(last - 1, last - 1, active);
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

  T* at(const size_t pos) {
    if(!_used || pos >= buffer_size ||  !active[pos]) return nullptr;

    return &(buffer[pos]);
  }

  const T* at(const size_t pos) const {
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
        new (&buffer[i]) T(std::forward<U>(item));
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
  T* replace(const size_t pos, U &&item) {
    if(!buffer_size || pos >= buffer_size || pos < 0) return nullptr;

    if(active[pos]) {
      buffer[pos] = item;
    } else {
      if(std::is_trivially_copyable<T>::value) {
        memcpy(&(buffer[pos]), &item, sizeof(T));
      } else {
        new (&buffer[pos]) T(std::forward<U>(item));
      }

      _used++;
    }

    if(pos >= _last) _last = pos + 1;

    return &(buffer[pos]);
  }

  template <typename U>
  T* replace(iterator pos, U &&item) {
    return replace(&*pos - buffer, item);
  }

  template <typename... Args>
  T* replace_new(const size_t pos, Args&&... args) {
    if(!buffer_size || pos >= buffer_size || pos < 0) return nullptr;

    if(active[pos]) {
      buffer[pos] = T(std::forward<Args>(args)...);
    } else {
      new (&buffer[pos]) T(std::forward<Args>(args)...);
      active[pos] = true;
      _used++;
    }

    if(pos >= _last) _last = pos + 1;

    return &(buffer[pos]);
  }

  template <typename... Args>
  T* replace_new(iterator pos, Args&&... args) {
    return replace_new(&*pos - buffer, args...);
  }

private:
  void insert_make_room_for_count(const size_t &position, const size_t& count) {
    if(position == _last) return;

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
      for(size_t i = _last - 1 + count; i >= (position + count); i--) {
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
    if(!count || _used + count > buffer_size || position > _last || position == buffer_size)
      return nullptr;

    compact();
    insert_make_room_for_count(position, count);
       
    for(size_t i = 0; i < count; i++) {
      active[_last + i] = true;

      if(std::is_trivially_copyable<T>::value)
        memcpy(&buffer[position + i], &item, sizeof(T));
      else
        new (&buffer[position + i]) T(std::forward<U>(item)); 
    }

    _last += count;
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
    if(!count || _used + count > buffer_size || position > _last || position == buffer_size)
      return nullptr;

    compact();
    insert_make_room_for_count(position, count);

    if(std::is_trivial<T>::value) {
      memcpy(&buffer[position], list.begin(), sizeof(T) * count);

      for(size_t i = 0; i < count; i++) {
        active[_last + i] = true;
      }
    } else {
      size_t i = 0;
      for(const auto &it : list) {
        active[_last + i] = true;

        new (&buffer[position + i]) T(std::move(it)); 

        i++;
      }
    }

    _last += count;
    _used += count;

    return &buffer[position];
  }

  T* insert(iterator pos, const std::initializer_list<T> list) {
    return insert(&*pos - buffer, list);
  }

  template <typename... Args>
  T* insert_new(size_t position, Args&&... args) {
    if(_used == buffer_size || position > _last || position == buffer_size)
      return nullptr;

    compact();
    insert_make_room_for_count(position, 1);

    active[position] = true;

    if(position != _last) active[_last] = true;

    new (&buffer[position]) T(std::forward<Args>(args)...);

    _last++;
    _used++;

    return &buffer[position];
  }

  template <typename... Args>
  T* insert_new(iterator pos, Args&&... args) {
    return insert_new(&*pos - buffer, args...);
  }

  template <typename U>
  T* push(U &&item) {
    if(_used == buffer_size) return nullptr;

    if(std::is_trivially_copyable<T>::value) {
      memcpy(&(buffer[_last]), &item, sizeof(T));
    } else {
      new (&buffer[_last]) T(std::forward<U>(item));
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

    if(!std::is_trivially_destructible<T>::value)
      buffer[_last - 1].~T();

    maybe_set_last(_last);
  }

  void erase_ptr(const T *item) {
    erase(((size_t)item - (size_t)buffer) / sizeof(T));
  }

  void erase(const size_t pos) {
    if(!buffer_size || pos <  0 || pos >= buffer_size || !active[pos]) return;

    _used--;
    active[pos] = false;

    if(!std::is_trivially_destructible<T>::value)
      buffer[pos].~T();

    maybe_set_last(pos + 1);
  }

  void reset() {
    for(size_t i = 0; i < buffer_size; i++) {
      if(active[i] && !std::is_trivially_destructible<T>::value)
        buffer[i].~T();

      active[i] = false;
    }

    _used = 0;
    _last = 0;
  }

  void compact() {
    if(!_used || _used == _last) return;

    size_t target = -1;
    for(size_t i = 0; i < buffer_size; i++) {
      if(target ==  _used) {
        break;
      } else if(target == -1) { 
        if(!active[i]) target = i;
      } else if(active[i]) {
        if(std::is_trivially_copyable<T>::value)
          memcpy(&(buffer[target]), &(buffer[i]), sizeof(T));
        else
          new (&buffer[target]) T(std::move(buffer[i]));

        if(!std::is_trivially_destructible<T>::value)
          buffer[i].~T();

        active[i] = false;
        active[target] = true;
        target++;
      }
    }

    if(target != -1) _last = target;
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

  Arena* arena() const {
    return nullptr;
  }
};


template <typename T, size_t N>
class SArrayFixed : public ISArray<T> {
  char static_buffer[sizeof(T) * N];
  bool static_active[N];

public:
  using ISArray<T>::operator=;

  SArrayFixed() : ISArray<T>() {
    this->buffer = reinterpret_cast<T*>(static_buffer);
    this->active = static_active;
    this->buffer_size = N;
  }

  SArrayFixed(const std::initializer_list<T> list) : SArrayFixed() {
    this->operator=(&list);
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  SArrayFixed(const std::vector<T>& other) : SArrayFixed() {
    this->operator=(other);
  }
  #endif

  SArrayFixed(const ISArray<T>& other) : SArrayFixed() {
    this->operator=(other);
  }

  ~SArrayFixed() {
    if(!std::is_trivially_destructible<T>::value) {
      for(size_t i = 0; i < this->buffer_size; i++) {
        if(this->active[i]) this->buffer[i].~T();
      }
    }
  }
};

template <typename T>
class SArray : public ISArray<T> {
  Arena* _arena;

protected:
  void moved_reset() {
    this->buffer = nullptr;
    this->active = nullptr;
    this->buffer_size = 0;
    this->_used = 0;
    this->_last = 0;
  }

public:
  using ISArray<T>::operator=;

  SArray(const size_t size = 0) : ISArray<T>(), _arena(nullptr) {
    init(size);
  }

  SArray(const std::initializer_list<T> list) :
    SArray(list.size())
  {
    this->operator=(&list);
  }

  SArray(const size_t size, const std::initializer_list<T> list) :
    SArray(size)
  {
    this->operator=(&list);
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  SArray(const std::vector<T>& other) :
    SArray(other.size())
  {
    this->operator=(other);
  }

  SArray(const size_t size, const std::vector<T>& other) :
    SArray(size)
  {
    this->operator=(other);
  }
  #endif

  // Move constructor.
  // (Can only move SArray, not SArrayFixed)
  SArray(SArray<T>&& other) : SArray() {
    if(other.arena()) _arena = other.arena();

    this->buffer = other.buffer;
    this->active = other.active;
    this->_used = other._used;
    this->buffer_size = other.buffer_size;
    this->_last = 5;

    other.moved_reset();
  }

  SArray(const SArray<T>& other) :
    SArray(other._arena, other.buffer_size)
  {
    this->operator=(other);
  }

  SArray(const ISArray<T>& other) :
    SArray(other.size())
  {
    this->operator=(other);
  }

  SArray(const size_t size, const ISArray<T>& other) :
    SArray(size)
  {
    this->operator=(other);
  }

  SArray(Arena* __arena, const size_t size) : ISArray<T>(), _arena(nullptr) {
    if(__arena) init(*__arena, size);
    else init(size);
  }

  SArray(Arena &__arena, const size_t size) : ISArray<T>(), _arena(nullptr) {
    init(__arena, size);
  }

  SArray(Arena &__arena, const size_t size, const std::initializer_list<T> list) :
    SArray(__arena, size) 
  {
    this->operator=(&list);
  }

  SArray(Arena &__arena, const size_t size, const std::vector<T>& other) :
    SArray(__arena, size) 
  {
    this->operator=(other);
  }

  SArray(Arena &__arena, const size_t size, const ISArray<T>& other) :
    SArray(__arena, size) 
  {
    this->operator=(other);
  }

  ~SArray() {
    if(!std::is_trivially_destructible<T>::value) {
      for(size_t i = 0; i < this->buffer_size; i++) {
        if(this->active[i]) this->buffer[i].~T();
      }
    }

    if(!_arena && this->buffer_size) {
      free(this->buffer);
      free(this->active);
    }
  }

  SArray& operator=(const SArray& other) = default;

  void init(const size_t size) {
    if(!size || _arena || this->buffer_size) return;

    this->buffer = static_cast<T*>(malloc(sizeof(T) * size));

    if(this->buffer)
      this->active = static_cast<bool*>(malloc(sizeof(bool) * size));

    if(!this->buffer || !this->active) {
      if(this->buffer) free(this->buffer);
      if(this->active) free(this->active);

      this->buffer = nullptr;
      this->active = nullptr;

      return;
    }

    this->buffer_size = size;
  }

  void init(Arena &__arena, const size_t size) {
    if(!size || _arena || this->buffer_size) return;

    auto* new_buffer = __arena.allocate_size<T>(size);
    bool* new_active = nullptr;

    if(new_buffer) new_active = __arena.allocate_size<bool>(size);

    if(new_active) {
      _arena = &__arena;
      this->buffer = new_buffer;
      this->active = new_active;

      this->buffer_size = size;
    }
  }

  bool resize(const size_t size) {
    this->compact();

    if(size == this->buffer_size) return true;

    T* new_buffer = nullptr;
    bool *new_active = nullptr;

    size_t copy_size = size > this->buffer_size ? this->buffer_size : size;

    if(_arena) {
      if(size < this->buffer_size) return false;

      new_buffer = _arena->allocate_size<T>(size);

      if(new_buffer)
        new_active = _arena->allocate_size<bool>(size);

      if(new_buffer && new_active) {
        if(std::is_trivially_copyable<T>::value)
          memcpy(new_buffer, this->buffer, sizeof(T) * copy_size);

        if(!std::is_trivially_destructible<T>::value ||
            !std::is_trivially_copyable<T>::value
        ) {
          for(size_t i = 0; i < this->buffer_size; i++) {
            if(!this->active[i]) break;

            if(!std::is_trivially_copyable<T>::value &&
              i < copy_size
            ) new (&new_buffer[i]) T(std::move(this->buffer[i]));

            if(!std::is_trivially_destructible<T>::value)
              this->buffer[i].~T();
          }
        }

        memcpy(new_active, this->active, sizeof(bool) * copy_size);
      } else {
        new_buffer = nullptr;
        new_active = nullptr;
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
        for(size_t i = 0; i < this->buffer_size; i++) {
            if(!this->active[i]) break;

            if(!std::is_trivially_copyable<T>::value && i < copy_size)
              new (&new_buffer[i]) T(std::move(this->buffer[i]));

            if(!std::is_trivially_destructible<T>::value)
              this->buffer[i].~T();
        }

        if(!std::is_trivially_copyable<T>::value)
          free(this->buffer);
      }

      if(new_buffer) {
        new_active = static_cast<bool*>(realloc(this->active, sizeof(bool) * size));
      }
    }

    if(new_buffer) this->buffer = new_buffer;

    if(new_active) {
      if(size > this->buffer_size) {
        for(size_t i = copy_size; i < size; i++) {
          new_active[i] = false;
        }
      }

      if(this->_used > size) {
        this->_used = size;
        this->_last = size;
      }

      this->active = new_active;
      this->buffer_size = size;

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

  Arena* arena() const {
    return _arena;
  }
};
