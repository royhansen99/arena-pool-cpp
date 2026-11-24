/*
 * Package: arena_pool_cpp
 * Version: 0.4.0
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
#include <type_traits>
#include <utility>
#include <new>

#ifndef APC_VECTOR_STD_VECTORS_DISABLE 
#include <vector>
#endif

namespace apc {

template <typename T, bool FORCE_TRIVIAL_COPY = false>
class ivector {
protected: 
  T* buffer;
  T* _end;
  T* next;

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

  ivector() : buffer(nullptr), _end(nullptr), next(nullptr) { }

  ivector& operator=(const std::initializer_list<T>& list) {
    if(!buffer) return *this;

    if(used()) reset();

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
      next = buffer + list.size();
      if(next > _end) next = _end;

      memcpy(buffer, list.begin(), used() * sizeof(T));
    } else {
      for(const auto& item : list)
        if(!push(item)) break;
    }

    return *this;
  }

  ivector& operator=(const ivector& other) {
    if(!buffer) return *this;

    if(used()) reset();

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
      next = buffer + other.used();
      if(next > _end) next = _end;

      memcpy(buffer, &*other.begin(), used() * sizeof(T));
    } else {
      for(const auto& item : other)
        if(!push(item)) break;
    }

    return *this;
  }

  #ifndef APC_VECTOR_STD_VECTORS_DISABLE
  ivector& operator=(const std::vector<T>& other) {
    if(!buffer) return *this;

    if(used()) reset();

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
      next = buffer + other.size();
      if(next > _end) next = _end;

      memcpy(buffer, &*other.begin(), used() * sizeof(T));
    } else {
      for(const auto& item : other)
        if(!push(item)) break;
    }

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
    return iterator(next ? next - 1 : next, true);
  }

  const iterator end() const {
    return iterator(next);
  }

  const iterator rend() const {
    return iterator(buffer - 1);
  }

  T* first() {
    return buffer;
  }

  const T* first() const {
    return buffer;
  }

  T* last() {
    return next ? next - 1 : nullptr;
  }

  const T* last() const {
    return next ? next - 1 : nullptr;
  }

  T& at(const size_t pos) {
    return buffer[pos];
  }

  const T& at(const size_t pos) const {
    return buffer[pos];
  }

  template <typename U>
  T& replace(const size_t pos, U&& item) {
    buffer[pos] = std::forward<U>(item);

    return buffer[pos];
  }

  template <typename U>
  T& replace(iterator pos, U&& item) {
    return replace(&*pos - buffer, std::forward<U>(item));
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
  void insert_make_room_for_count(const size_t& position, const size_t& count) {
    size_t used = this->used();

    if(position == used) return;

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
      memmove(
        &buffer[position + count],
        &buffer[position],
        sizeof(T) * (used - position)
      );
    } else {
      for(auto* target = buffer + position; target != next; target++) {
         new (target + count) T(std::move(*target));

        if(!std::is_trivially_destructible<T>::value)
          target->~T();
      }
    }
  }

public:
  template <typename U>
  T* insert(size_t position, const size_t count, U&& item) {
    size_t used = this->used();
    if(!count || used + count > size() || position > used || position == size())
      return nullptr;

    insert_make_room_for_count(position, count);
       
    for(size_t i = 0; i < count; i++) {
      if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
        memcpy(&buffer[position + i], &item, sizeof(T));
      else if(i == count - 1)
        new (&buffer[position + i]) T(std::forward<U>(item)); 
      else
        new (&buffer[position + i]) T(item); 
    }

    next += count;

    return &buffer[position];
  }

  template <typename U>
  T* insert(size_t position, U&& item) {
    return insert(position, 1, std::forward<U>(item));
  }

  template <typename U>
  T* insert(iterator pos, const size_t count, U&& item) {
    return insert(&*pos - buffer, count, std::forward<U>(item));
  }

  template <typename U>
  T* insert(iterator pos, U&& item) {
    return insert(pos, 1, std::forward<U>(item));
  }

  T* insert(size_t position, const std::initializer_list<T>& list) {
    size_t count = list.size();

    if(!count || used() + count > size() || position > used() || position == size())
      return nullptr;

    insert_make_room_for_count(position, count);

    if(std::is_trivial<T>::value) {
      memcpy(&buffer[position], list.begin(), sizeof(T) * count);
    } else {
      size_t i = 0;
      for(const auto& it : list) {
        new (&buffer[position + i]) T(it); 

        i++;
      }
    }

    next += count;

    return &buffer[position];
  }

  T* insert(iterator pos, const std::initializer_list<T>& list) {
    return insert(&*pos - buffer, list);
  }

  template <typename... Args>
  T* insert_new(size_t position, Args&&... args) {
    size_t used = this->used();
    size_t size = this->size();

    if(used == size || position > used || position == size)
      return nullptr;

    insert_make_room_for_count(position, 1);

    new (&buffer[position]) T(std::forward<Args>(args)...);

    next++;

    return &buffer[position];
  }

  template <typename... Args>
  T* insert_new(iterator pos, Args&&... args) {
    return insert_new(&*pos - buffer, args...);
  }

  template <typename U>
  T* push(U&& item) {
    if(next == _end) return nullptr;

    if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) {
      memcpy(next, &item, sizeof(T));
    } else {
      new (next) T(std::forward<U>(item));
    }

    return next++;
  }

  template <typename... Args>
  T* push_new(Args&&... args) {
    if(next == _end) return nullptr;

    new (next) T(std::forward<Args>(args)...);

    return next++;
  }

  void pop() {
    if(next != buffer) erase_ptr(next - 1);
  }

  void erase_ptr(T* item) {
    if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY)
      item->~T();

    if(item + 1 != next) {
      if(FORCE_TRIVIAL_COPY || std::is_trivially_copyable<T>::value) {
        memmove(item, item + 1, sizeof(T) * (next - item - 1));
      } else {
        for(auto* i = item + 1; i < next; i++) {
          new (i - 1) T(std::move(*i));

          if(!std::is_trivially_destructible<T>::value)
            i->~T();
        }
      }
    }

    next--;
  }

  void erase(const size_t pos) {
    erase_ptr(buffer + pos);
  }

  void reset() {
    if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) {
      for(auto* it = buffer; it != next; it++)
        it->~T();
    }

    next = buffer;
  }

  size_t used() const {
    return next - buffer;
  }

  size_t size() const {
    return _end - buffer;
  }

  bool empty() const {
    return next == buffer;
  }

  #ifdef APC_ARENA
  arena* arena() const {
    return nullptr;
  }
  #endif
};

template <typename T, bool Trivial>
class vector_fixed_mixin : public ivector<T> {
  ~vector_fixed_mixin() {
    for(auto* it = this->buffer; it != this->next; it++)
      it->~T();
  }
};

template <typename T>
class vector_fixed_mixin<T, true> {
};

template <typename T, size_t N, bool FORCE_TRIVIAL_COPY = false>
class vector_fixed final : public ivector<T>, public vector_fixed_mixin<T, std::is_trivially_destructible<T>::value> {
  char static_buffer[sizeof(T) * N];

public:
  using ivector<T>::operator=;

  vector_fixed() : ivector<T>() {
    this->buffer = reinterpret_cast<T*>(static_buffer);
    this->_end = this->buffer + N;
    this->next = this->buffer;
  }

  vector_fixed(const std::initializer_list<T>& list) : vector_fixed() {
    this->operator=(list);
  }

  #ifndef APC_VECTOR_STD_VECTORS_DISABLE
  vector_fixed(const std::vector<T>& other) : vector_fixed() {
    this->operator=(other);
  }
  #endif

  vector_fixed(const ivector<T>& other) : vector_fixed() {
    this->operator=(other);
  }
};

template <typename T, bool FORCE_TRIVIAL_COPY = false>
class vector final : public ivector<T> {
  #ifdef APC_ARENA
  apc::arena* _arena;
  #endif

  void free_resources() {
    if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) {
      for(auto* it = this->buffer; it != this->next; it++)
        it->~T();
    }

    if(
        #ifdef APC_ARENA
        !_arena &&
        #endif
        this->size() 
    )
      free(this->buffer);

    #ifdef APC_ARENA
    this->_arena = nullptr;
    #endif
    this->buffer = nullptr;
    this->next = nullptr;
    this->_end = nullptr;
  }

  void destroy() {
    #ifdef APC_ARENA
    _arena = nullptr;
    #endif

    this->buffer = nullptr;
    this->next = nullptr;
    this->_end = nullptr;
  }

  void maybe_grow(size_t count) {
    if(count) {
      if(!this->buffer) {
        this->init(count);
      } else if(this->size() - this->used() < count) {
        size_t new_size = this->size() * 2;
        size_t min = this->used() + count; 

        if(new_size < min) new_size = min;

        this->resize(new_size);
      }
    }
  }

  void maybe_grow() {
    if(this->next == this->_end) {
      if(!this->buffer) {
        this->init(1);
      } else {
        this->resize(this->used() * 2);
      }
    }
  }

public:
  vector(const size_t size = 0) :
    #ifdef APC_ARENA
    ivector<T>(),
    _arena(nullptr)
    #else
    ivector<T>()
    #endif
  {
    init(size);
  }

  vector(const std::initializer_list<T>& list) :
    vector(list.size())
  {
    this->operator=(list);
  }

  vector(const size_t size, const std::initializer_list<T>& list) :
    vector(size)
  {
    this->operator=(list);
  }

  #ifndef APC_VECTOR_STD_VECTORS_DISABLE
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

  vector(vector&& other) {
    #ifdef APC_ARENA
    if(other.arena()) _arena = other.arena();
    #endif

    this->buffer = other.buffer;
    this->_end = other._end;
    this->next = other.next;

    other.destroy();
  }

  vector(const vector<T>& other) : vector()
  {
    this->operator=(other);
  }

  vector(const ivector<T>& other) :
    vector(other.used())
  {
    this->operator=(other);
  }

  vector(const size_t size, const ivector<T>& other) :
    vector(size)
  {
    this->operator=(other);
  }

  #ifdef APC_ARENA
  vector(apc::arena& __arena, const size_t size) : ivector<T>(), _arena(nullptr) {
    init(__arena, size);
  }

  vector(apc::arena& __arena, const size_t size, const std::initializer_list<T>& list) :
    vector(__arena, size) 
  {
    this->operator=(list);
  }

  vector(apc::arena& __arena, const size_t size, const std::vector<T>& other) :
    vector(__arena, size) 
  {
    this->operator=(other);
  }

  vector(apc::arena& __arena, const size_t size, const ivector<T>& other) :
    vector(__arena, size) 
  {
    this->operator=(other);
  }
  #endif

  ~vector() {
    free_resources();
  }

  vector& operator=(const vector& other) {
    this->reset();
    maybe_grow(other.used());
    ivector<T>::operator=(other);

    return *this;
  }

  vector& operator=(vector&& other) {
    free_resources();

    #ifdef APC_ARENA
    if(other.arena()) _arena = other.arena();
    #endif

    this->buffer = other.buffer;
    this->_end = other._end;
    this->next = other.next;

    other.destroy();

    return *this;
  }

  vector& operator=(const std::initializer_list<T>& list) {
    this->reset();
    maybe_grow(list.size());
    ivector<T>::operator=(list);

    return *this;
  }

  vector& operator=(const ivector<T>& other) {
    this->reset();
    maybe_grow(other.used());
    ivector<T>::operator=(other);

    return *this;
  }

  #ifndef APC_VECTOR_STD_VECTORS_DISABLE
  vector& operator=(const std::vector<T>& other) {
    this->reset();
    maybe_grow(other.size());
    ivector<T>::operator=(other);

    return *this;
  }
  #endif

  template <typename U>
  T* insert(size_t position, const size_t count, U&& item) {
    maybe_grow(count);
    return ivector<T>::insert(position, count, std::forward<U>(item));
  }

  template <typename U>
  T* insert(size_t position, U&& item) {
    return insert(position, 1, std::forward<U>(item));
  }

  template <typename U>
  T* insert(vector::iterator pos, const size_t count, U&& item) {
    return insert(&*pos - this->buffer, count, std::forward<U>(item));
  }

  template <typename U>
  T* insert(vector::iterator pos, U&& item) {
    return insert(pos, 1, std::forward<U>(item));
  }

  T* insert(size_t position, const std::initializer_list<T>& list) {
    maybe_grow(list.size());
    return ivector<T>::insert(position, list);
  }

  T* insert(vector::iterator pos, const std::initializer_list<T>& list) {
    return insert(&*pos - this->buffer, list);
  }

  template <typename... Args>
  T* insert_new(size_t position, Args&&... args) {
    maybe_grow();
    return ivector<T>::insert_new(position, std::forward<Args>(args)...);
  }

  template <typename... Args>
  T* insert_new(vector::iterator pos, Args&&... args) {
    return insert_new(&*pos - this->buffer, args...);
  }

  template <typename U>
  T* push(U&& item) {
    maybe_grow();
    return ivector<T>::push(std::forward<T>(item));
  }

  template <typename... Args>
  T* push_new(Args&&... args) {
    maybe_grow();
    return ivector<T>::push_new(std::forward<Args>(args)...);
  }

  void init(const size_t size) {
    if(
      !size ||
      #ifdef APC_ARENA
      _arena ||
      #endif
      this->size()
    ) return;

    this->buffer = static_cast<T*>(malloc(sizeof(T) * size));

    if(this->buffer) {
      this->_end = this->buffer + size;
      this->next = this->buffer;
    }
  }

  #ifdef APC_ARENA
  void init(apc::arena& __arena, const size_t size) {
    if(!size || _arena || this->size()) return;

    auto* new_buffer = __arena.allocate_size<T>(size);

    if(new_buffer) {
      _arena = &__arena;
      this->buffer = new_buffer;
      this->_end = this->buffer + size;
      this->next = this->buffer;
    }
  }
  #endif

  bool resize(const size_t size) {
    if(size == this->size()) return true;

    T* new_buffer = nullptr;
    size_t used = this->used();
    size_t copy_size = size > used ? used : size;

    #ifdef APC_ARENA
    if(_arena) {
      if(size < this->size()) {
        if(used > size) {
          if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) {
            for(size_t i = size - 1; i < used; i++) {
              this->buffer[i].~T();
            }
          }

          this->next = this->buffer + size;
        }

        this->_end = this->buffer + size;

        return true;
      } else {
        new_buffer = _arena->allocate_size<T>(size);

        if(new_buffer) {
          if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
            memcpy(new_buffer, this->buffer, sizeof(T) * copy_size);
          else { 
            for(size_t i = 0; i < used; i++) {
              if(i < copy_size) new (&new_buffer[i]) T(std::move(this->buffer[i]));

              if(!std::is_trivially_destructible<T>::value)
                this->buffer[i].~T();
            }
          }
        }
      }
    } else {
    #else
    {
    #endif
      if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY)
        new_buffer = static_cast<T*>(
          realloc(this->buffer, sizeof(T) * size)
        );
      else {
        new_buffer = static_cast<T*>(malloc(sizeof(T) * size));

        if(new_buffer) {
          for(size_t i = 0; i < used; i++) {
              if(i < copy_size)
                new (&new_buffer[i]) T(std::move(this->buffer[i]));

              if(!std::is_trivially_destructible<T>::value)
                this->buffer[i].~T();
          }

          free(this->buffer);
        }
      }
    }

    if(new_buffer) {
      this->buffer = new_buffer;
      this->_end = this->buffer + size;
      this->next = this->buffer + copy_size;

      return true;
    }

    return false;
  }

  bool shrink_to_fit() {
    size_t used = this->used();
    size_t size = this->size();

    if(used < size && size > 1)
      return resize(used ? used : 1);

    return 0;
  }

  #ifdef APC_ARENA
  apc::arena* arena() const {
    return _arena;
  }
  #endif
};

}
