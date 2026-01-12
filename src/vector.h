/*
 * Package: arena_pool_cpp
 * Version: 0.2.1
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

#ifndef SARRAY_STD_VECTORS_DISABLE 
  #include <vector>
#endif

namespace apc {

template <typename T>
class ivector {
protected: 
  T* buffer;
  size_t buffer_size;
  size_t _used;

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
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : list)
      if(!push(item)) break;

    return *this;
  }

  ivector& operator=(const std::initializer_list<T>* list) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : *list)
      if(!push(item)) break;

    return *this;
  }

  ivector& operator=(const ivector& other) {
    if(!buffer_size) return *this;

    if(_used) reset();

    for(const auto item : other)
      if(!push(item)) break;

    return *this;
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  ivector& operator=(const std::vector<T>& other) {
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

  #ifdef ARENA_POOL_CPP
  arena* arena() const {
    return nullptr;
  }
  #endif
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
  #ifdef ARENA_POOL_CPP
  arena* _arena;
  #endif

protected:
  void moved_reset() {
    this->buffer = nullptr;
    this->buffer_size = 0;
    this->_used = 0;
  }

  void maybe_grow(const size_t &count) {
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
  vector(const size_t size = 0) :
    #ifdef ARENA_POOL_CPP
    ivector<T>(),
    _arena(nullptr)
    #else
    ivector<T>()
    #endif
  {
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
    #ifdef ARENA_POOL_CPP
    if(other.arena()) _arena = other.arena();
    #endif

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

  #ifdef ARENA_POOL_CPP
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
  #endif

  ~vector() {
    if(!std::is_trivially_destructible<T>::value) {
      for(size_t i = 0; i < this->_used; i++)
        this->buffer[i].~T();
    }

    if(
        #ifdef ARENA_POOL_CPP
        !_arena &&
        #endif
        this->buffer_size
    )
      free(this->buffer);
  }

  vector& operator=(const vector& other) {
    maybe_grow(other.used());
    ivector<T>::operator=(other);

    return *this;
  }

  vector& operator=(const std::initializer_list<T> list) {
    maybe_grow(list.size());
    ivector<T>::operator=(list);

    return *this;
  }

  vector& operator=(const std::initializer_list<T>* list) {
    maybe_grow(list->size());
    ivector<T>::operator=(list);

    return *this;
  }

  vector& operator=(const ivector<T>& other) {
    maybe_grow(other.used());
    ivector<T>::operator=(other);

    return *this;
  }

  #ifndef SARRAY_STD_VECTORS_DISABLE
  vector& operator=(const std::vector<T>& other) {
    maybe_grow(other.size());
    ivector<T>::operator=(other);

    return *this;
  }
  #endif

  template <typename U>
  T* insert(size_t position, const size_t count, U &&item) {
    maybe_grow(count);
    return ivector<T>::insert(position, count, item);
  }

  template <typename U>
  T* insert(size_t position, U &&item) {
    return insert(position, 1, item);
  }

  template <typename U>
  T* insert(vector::iterator pos, const size_t count, U &&item) {
    return insert(&*pos - this->buffer, count, item);
  }

  template <typename U>
  T* insert(vector::iterator pos, U &&item) {
    return insert(pos, 1, item);
  }

  T* insert(size_t position, const std::initializer_list<T> list) {
    maybe_grow(list.size());
    return ivector<T>::insert(position, list);
  }

  T* insert(vector::iterator pos, const std::initializer_list<T> list) {
    return insert(&*pos - this->buffer, list);
  }

  template <typename... Args>
  T* insert_new(size_t position, Args&&... args) {
    maybe_grow(1);
    return ivector<T>::insert_new(position, std::forward<Args>(args)...);
  }

  template <typename... Args>
  T* insert_new(vector::iterator pos, Args&&... args) {
    return insert_new(&*pos - this->buffer, args...);
  }

  template <typename U>
  T* push(U &&item) {
    maybe_grow(1);
    return ivector<T>::push(item);
  }

  template <typename... Args>
  T* push_new(Args&&... args) {
    maybe_grow(1);
    return ivector<T>::push_new(std::forward<Args>(args)...);
  }

  void init(const size_t size) {
    if(
      !size ||
      #ifdef ARENA_POOL_CPP
      _arena ||
      #endif
      this->buffer_size
    ) return;

    this->buffer = static_cast<T*>(malloc(sizeof(T) * size));

    if(this->buffer) this->buffer_size = size;
  }

  #ifdef ARENA_POOL_CPP
  void init(arena &__arena, const size_t size) {
    if(!size || _arena || this->buffer_size) return;

    auto* new_buffer = __arena.allocate_size<T>(size);

    if(new_buffer) {
      _arena = &__arena;
      this->buffer = new_buffer;
      this->buffer_size = size;
    }
  }
  #endif

  bool resize(const size_t size) {
    if(size == this->buffer_size) return true;

    T* new_buffer = nullptr;
    size_t copy_size = size > this->buffer_size ? this->buffer_size : size;

    #ifdef ARENA_POOL_CPP
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
    #else
    {
    #endif
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

  #ifdef ARENA_POOL_CPP
  arena* arena() const {
    return _arena;
  }
  #endif
};

}
