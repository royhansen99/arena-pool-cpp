/*
 * Package: arena_pool_cpp
 * Version: 0.3.4
 * License: MIT
 * Github: https://github.com/royhansen99/arena-pool-cpp 
 * Author: Roy Hansen (https://github.com/royhansen99)
 * Description: A very fast memory allocator library with an arena/bump-allocator,
 *              a pool-allocator, a hashmap-allocator,  and a vector-like
 *              array-allocator. Also includes a string implementation.
 *              Single header c++11 library.
 */

/*
 * This is a Red-Black Tree (CLRS) implementation, written based on examples
 * found in the book "Introduction to Algorithms 4th edition (CLRS)".
 * Pointers are stable regardless of rebalancing/rotations. 
 */

#pragma once

#include <cstddef>
#include <functional>
#include <cstring>
#include <new>

namespace apc {

template <typename T, typename K>
struct treemap_node {
  T current;
  K key;
  bool is_red = false; // (Red-black binary tree) 
  treemap_node<T, K>* parent = nullptr;
  treemap_node<T, K>* left = nullptr;
  treemap_node<T, K>* right = nullptr;
};

template <typename T, typename K, size_t S>
struct treemap_page_fixed {
  treemap_node<T, K> buffer[S];
  size_t size = S;
};

template <typename T, typename K>
struct treemap_page {
  treemap_node<T, K>* buffer;
  size_t size;
};

#define APC_TREEMAP_COMMON \
  private: \
    treemap_node<T, K>* root = nullptr; \
    treemap_node<T, K>* first = nullptr; \
    treemap_node<T, K>* last = nullptr; \
    \
    treemap_node<T, K>* free_ptr = nullptr; \
    size_t _used = 0; \
    \
    void deallocate(treemap_node<T, K>* node) { \
      if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) \
        node->current.~T(); \
      \
      node->right = free_ptr; \
      free_ptr = node; \
      _used--; \
    } \
    \
    void left_rotate(treemap_node<T, K>* x) { \
      auto* y = x->right; \
      \
      if(y == nullptr) return; \
      \
      x->right = y->left; \
      \
      if(y->left != nullptr) y->left->parent = x; \
      \
      y->parent = x->parent; \
      \
      if(x->parent == nullptr) root = y; \
      else if(x == x->parent->left) x->parent->left = y; \
      else x->parent->right = y; \
      \
      y->left = x; \
      x->parent = y; \
    } \
    \
    void right_rotate(treemap_node<T, K>* x) { \
      auto* y = x->left; \
      \
      if(y == nullptr) return; \
      \
      x->left = y->right; \
      \
      if(y->right != nullptr) y->right->parent = x; \
      \
      y->parent = x->parent; \
      \
      if(x->parent == nullptr) root = y; \
      else if(x == x->parent->right) x->parent->right = y; \
      else x->parent->left = y; \
      \
      y->right = x; \
      x->parent = y; \
    } \
    \
    void insert_fixup(treemap_node<T, K>* z) { \
      if(z->parent == nullptr) return; \
      \
      while(z != nullptr && z->parent != nullptr && z->parent->is_red) { \
        if(z->parent->parent != nullptr && z->parent == z->parent->parent->left) { \
          auto* y = z->parent->parent->right; \
          \
          if(y != nullptr && y->is_red) { \
            z->parent->is_red = false; \
            y->is_red = false; \
            z->parent->parent->is_red = true; \
            z = z->parent->parent; \
          } else { \
            if(z == z->parent->right) { \
              z = z->parent; \
              left_rotate(z->parent->parent); \
            } \
            \
            z->parent->is_red = false; \
            z->parent->parent->is_red = true; \
            \
            right_rotate(z->parent->parent); \
          } \
        } else  { \
          auto* y = z->parent->parent != nullptr ? \
            z->parent->parent->left : nullptr; \
          \
          if(y != nullptr && y->is_red) { \
            z->parent->is_red = false; \
            y->is_red = false; \
            z->parent->parent->is_red = true; \
            z = z->parent->parent; \
          } else { \
            if(z == z->parent->left) { \
              z = z->parent; \
              \
              right_rotate(z); \
            } \
            \
            z->parent->is_red = false; \
            \
            if(z->parent->parent != nullptr) { \
              z->parent->parent->is_red = true; \
              \
              left_rotate(z->parent->parent); \
            } \
          } \
        } \
      } \
      \
      root->is_red = false; \
    } \
    \
    void transplant(treemap_node<T, K>* u, treemap_node<T, K>* v) { \
      if(u->parent == nullptr) root = v; \
      else if(u == u->parent->left) u->parent->left = v; \
      else u->parent->right = v; \
      \
      if(v != nullptr) v->parent = u->parent; \
    } \
    \
    void delete_fixup(treemap_node<T, K>* x, treemap_node<T, K>* p, bool x_is_left) { \
      while(x != root && (x == nullptr || !x->is_red)) { \
        if(x_is_left) { \
          auto* w = p->right; \
          \
          if(w != nullptr && w->is_red) { \
            w->is_red = false; \
            p->is_red = true; \
            \
            left_rotate(p); \
            \
            w = p->right; \
          } \
          \
          if( \
            w != nullptr \
            && (w->left == nullptr || !w->left->is_red) \
            && (w->right == nullptr || !w->right->is_red) \
          ) { \
            w->is_red = true; \
            x = p; \
            p = p->parent; \
          } else { \
            if( \
              w != nullptr \
              && (w->right == nullptr || !w->right->is_red) \
            ) { \
              if(w->left != nullptr) w->left->is_red = false; \
              \
              w->is_red = true; \
              \
              right_rotate(w); \
              \
              w = p->right; \
            } \
            \
            if(w != nullptr) w->is_red = p->is_red; \
            \
            p->is_red = false; \
            \
            if(w != nullptr && w->right != nullptr) \
              w->right->is_red = false; \
            \
            left_rotate(p); \
            \
            x = root; \
          } \
        } else { \
          auto* w = p->left; \
          \
          if(w != nullptr && w->is_red) { \
            w->is_red = false; \
            p->is_red = true; \
            \
            right_rotate(p); \
            \
            w = p->right; \
          } \
          \
          if( \
            w != nullptr \
            && (w->left == nullptr || !w->left->is_red) \
            && (w->right == nullptr || !w->right->is_red) \
          ) { \
            w->is_red = true; \
            x = p; \
            p = p->parent; \
          } else { \
            if( \
              w != nullptr \
              && (w->left == nullptr || !w->left->is_red) \
            ) { \
              if(w->right != nullptr) w->right->is_red = false; \
              \
              w->is_red = true; \
              \
              left_rotate(w); \
              \
              w = p->left; \
            } \
            \
            if(w != nullptr) w->is_red = p->is_red; \
            \
            p->is_red = false; \
            \
            if(w != nullptr && w->left != nullptr) \
              w->left->is_red = false; \
            \
            right_rotate(p); \
            \
            x = root; \
          } \
        } \
      } \
    } \
    \
    static treemap_node<T, K>* tree_minimum(treemap_node<T, K>* x) { \
      while(x->left != nullptr) \
        x = x->left; \
      \
      return x; \
    } \
    \
    static treemap_node<T, K>* tree_maximum(treemap_node<T, K>* x) { \
      while(x->right != nullptr) \
        x = x->right; \
      \
      return x; \
    } \
    \
    treemap_node<T, K>* find_item(K& key) { \
      auto* node = root; \
      \
      while(node != nullptr) { \
        if(node->key == key) return node; \
        \
        node = COMPARE{}(key, node->key) ? \
          node->left : node->right; \
      } \
      \
      return nullptr; \
    } \
    \
  public: \
    void reset() { \
      if(!pages_size) return; \
      \
      if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) { \
        for(auto it = begin(); it != end(); ++it) { \
          (*it).~T(); \
        } \
      } \
      \
      treemap_node<T, K>* previous = nullptr; \
      for(size_t i = 0; i < pages_size; i++) { \
        for(size_t z = 0; z < pages[i].size; z++) { \
          if(previous != nullptr) previous->right = &(pages[i].buffer[z]); \
          \
          previous = &(pages[i].buffer[z]); \
          previous->right = nullptr; \
        } \
      } \
      \
      previous->right = nullptr; \
      free_ptr = &(pages[0].buffer[0]); \
      \
      _used = 0; \
      root = nullptr; \
      first = nullptr; \
      last = nullptr; \
    } \
    class iterator { \
      treemap_node<T, K>* it; \
      bool _reverse; \
    public: \
      explicit iterator(treemap_node<T, K>* begin, bool reverse = false): \
        it(begin), _reverse(reverse) { } \
      \
      T& operator*() const { return it->current; } \
      \
      bool operator!=(const iterator& other) const { return it != other.it; } \
      \
      bool operator!=(const void* other) const { return it != other; } \
      \
      iterator& operator+(size_t count) { \
        while(count > 0 && it != nullptr) { \
          count--; \
          operator++(); \
        } \
        \
        return *this; \
      } \
      \
      iterator& operator++() { \
        auto* orig_it = it; \
        \
        if(_reverse) { \
          if(it->left != nullptr) it = tree_maximum(it->left); \
          else if(it->parent != nullptr) { \
            while(it != nullptr) { \
              if(it->parent->left == it && it->parent->parent == nullptr) { \
                it = nullptr; \
                break; \
              } \
              \
              bool stop = it->parent->right == it; \
              it = it->parent; \
              \
              if(stop) break; \
            } \
          } \
        } else { \
          if(it->right != nullptr) it = tree_minimum(it->right); \
          else if(it->parent != nullptr) { \
            while(it != nullptr) { \
              if(it->parent != nullptr && it->parent->right == it && it->parent->parent == nullptr) { \
                it = nullptr; \
                break; \
              } \
              \
              bool stop = it->parent == nullptr || it->parent->left == it; \
              \
              it = it->parent; \
              \
              if(stop) break; \
            } \
          } \
        } \
        \
        if(orig_it == it) it = nullptr; \
        \
        return *this; \
      } \
    }; \
    \
    iterator begin() const { \
      return iterator(first); \
    } \
    \
    iterator rbegin() const { \
      return iterator(last, true); \
    } \
    \
    const iterator end() const { \
      return iterator(nullptr); \
    } \
    \
    const iterator rend() const { \
      return iterator(nullptr); \
    } \
    \
    bool insert(K& key, T& item) { \
      auto* z = allocate(); \
      \
      if(z == nullptr) return false; \
      \
      if(std::is_trivially_copyable<T>::value || FORCE_TRIVIAL_COPY) { \
        memcpy(&z->current, &item, sizeof(T)); \
      } else { \
        new (&z->current) T(std::forward<T>(item));  \
      } \
      \
      if(std::is_trivially_copyable<K>::value) { \
        memcpy(&z->key, &key, sizeof(K)); \
      } else { \
        new (&z->key) K(std::forward<K>(key));  \
      } \
      \
      auto* x = root; \
      treemap_node<T, K>* y = nullptr; \
      \
      while(x != nullptr) { \
        y = x; \
        \
        if(COMPARE{}(z->key, x->key)) x = x->left; \
        else x = x->right; \
      } \
      \
      z->parent = y; \
      \
      if(y == nullptr) root = z; \
      else if(COMPARE{}(z->key, y->key)) y->left = z; \
      else y->right = z; \
      \
      z->left = nullptr; \
      z->right = nullptr; \
      z->is_red = true; \
      \
      insert_fixup(z); \
      \
      if(first == nullptr || COMPARE{}(z->key, first->key)) \
        first = z; \
      \
      if(last == nullptr || COMPARE{}(last->key, z->key)) \
        last = z; \
      \
      return true; \
    } \
    \
    bool insert(K&& key, T& item) { \
      return insert(key, item); \
    } \
    \
    bool insert(K&& key, T&& item) { \
      return insert(key, item); \
    } \
    \
    bool insert(K& key, T&& item) { \
      return insert(key, item); \
    } \
    \
    bool erase(K& key) { \
      auto* z = find_item(key); \
      \
      if(z == nullptr) return false; \
      \
      if(z == first && z == last) { \
        first = nullptr; \
        last = nullptr; \
      } else if(z == first) { \
        first = \
          reinterpret_cast<treemap_node<T, K>*>( \
              &*(++iterator(z)) \
          ); \
      } else if(z == last) { \
        last = \
          reinterpret_cast<treemap_node<T, K>*>( \
              &*(++iterator(z, true)) \
          ); \
      } \
      \
      auto* y = z; \
      bool y_original_is_red = y->is_red; \
      \
      treemap_node<T, K>* x = nullptr; \
      treemap_node<T, K>* x_parent = nullptr; \
      bool x_is_left; \
      \
      if(z->left == nullptr) { \
        x = z->right; \
        x_parent = z; \
        x_is_left = false; \
        \
        transplant(z, z->right); \
      } else if(z->right == nullptr) { \
        x = z->left; \
        x_parent = z; \
        x_is_left = true; \
        \
        transplant(z, z->left); \
      } else { \
        y = tree_minimum(z->right); \
        y_original_is_red = y->is_red; \
        \
        x = y->right; \
        x_parent = y; \
        x_is_left = false; \
        \
        if(y != z->right) { \
          transplant(y, y->right); \
          \
          y->right = z->right; \
          y->right->parent = y; \
        } else if(x != nullptr && x->parent != nullptr) { \
          x->parent = y; \
        } \
        \
        transplant(z, y); \
        \
        y->left = z->left; \
        z->left->parent = y; \
        y->is_red = z->is_red; \
      } \
      \
      if(!y_original_is_red) delete_fixup(x, x_parent, x_is_left); \
      \
      deallocate(z); \
      \
      return true; \
    } \
    \
    bool erase(K&& key) { \
      return erase(key); \
    } \
    \
    bool erase(iterator it) { \
      if(!(it != nullptr)) return false; \
      \
      return erase( \
        (reinterpret_cast<treemap_node<T, K>*>(&(*it)))->key \
      ); \
      \
      return true; \
    } \
    \
    T* find(K& key) { \
      auto* node = find_item(key); \
      \
      return node != nullptr ? &node->current : nullptr; \
    } \
    \
    T* find(K&& key) { \
      return find(key); \
    } \
    \
    size_t used() const { return _used; } \
    \
    size_t size() const { \
      size_t total_size = 0; \
      \
      for(size_t i = 0; i < pages_size; i++) { \
        total_size += pages[i].size; \
      } \
      \
      return total_size; \
    }

template <typename T, typename K, size_t S, typename COMPARE = std::less<K>, bool FORCE_TRIVIAL_COPY = false>
class treemap_fixed {
private:
  treemap_page_fixed<T, K, S> pages[1];  
  size_t pages_size = 1;

  treemap_node<T, K>* allocate() {
    if(free_ptr == nullptr) return nullptr;

    auto* item = free_ptr;
    free_ptr = item->right;
    _used++;

    return item;
  }

public:
  treemap_fixed() {
    reset();
  }

  ~treemap_fixed() {
    if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) {
      for(auto it = begin(); it != end(); ++it) {
        (*it).~T();
      }
    }
  }

  APC_TREEMAP_COMMON

#ifdef APC_TREEMAP_TESTING 
  treemap_node<T, K>* get_root() { return root; }
#endif
};

template <typename T, typename K, typename COMPARE = std::less<K>,
  bool FORCE_TRIVIAL_COPY = false>
class treemap {
private:
  #ifdef ARENA_POOL_CPP
  apc::arena* arena = nullptr;
  #endif

  treemap_page<T, K>* pages = nullptr;  
  size_t pages_size = 0;

  treemap_node<T, K>* allocate() {
    if(free_ptr == nullptr) {
      if(!grow(size() ? size() * 2 : 1)) return nullptr;
    }

    auto* item = free_ptr;
    free_ptr = item->right;
    _used++;

    return item;
  }

public:
  treemap(const size_t treemap_size = 0) {
    if(treemap_size) grow(treemap_size);
  }

  #ifdef ARENA_POOL_CPP
  treemap(apc::arena& _arena, const size_t treemap_size = 0) : arena(&_arena) {
    if(treemap_size) grow(treemap_size);
  }
  #endif

  ~treemap() {
    if(!std::is_trivially_destructible<T>::value && !FORCE_TRIVIAL_COPY) {
      for(auto it = begin(); it != end(); ++it) {
        (*it).~T();
      }
    }

    #ifdef ARENA_POOL_CPP
    if(!arena) {
    #endif
      for(size_t i = 0; i < pages_size; i++) {
        free(pages[i].buffer);
      }

      if(pages != nullptr) free(pages);
    #ifdef ARENA_POOL_CPP
    }
    #endif
  }

  bool grow(const size_t size) {
    treemap_node<T, K>* new_buffer = nullptr;
    size_t new_count = pages_size + 1;

    #ifdef ARENA_POOL_CPP
    if(arena)
      new_buffer = arena->allocate_size<treemap_node<T, K>>(size);
    else
    #endif
      new_buffer = static_cast<treemap_node<T, K>*>(
        malloc(sizeof(treemap_node<T, K>) * size)
      );

    if(!new_buffer) return false;

    treemap_page<T, K>* new_list = nullptr;

    #ifdef ARENA_POOL_CPP
    if(arena)
      new_list = arena->allocate_size<treemap_page<T, K>>(new_count);
    else if(pages_size)
      new_list = static_cast<treemap_page<T, K>*>(
        realloc(pages, sizeof(treemap_page<T, K>) * new_count)
      );
    else
      new_list = static_cast<treemap_page<T, K>*>(
        malloc(sizeof(treemap_page<T, K>) * new_count)
      );
    #else
    if(pages_size)
      new_list = static_cast<treemap_page<T, K>*>(
        realloc(pages, sizeof(treemap_page<T, K>) * new_count)
      );
    else
      new_list = static_cast<treemap_page<T, K>*>(
        malloc(sizeof(treemap_page<T, K>) * new_count)
      );
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
      memcpy(new_list, pages, sizeof(treemap_page<T, K>) * pages_size);
    #endif

    new_list[new_count - 1] = { new_buffer, size };
    pages = new_list;
    pages_size = new_count;

    for(size_t i = 1; i < size; i++) {
      new_buffer[i - 1].right = &(new_buffer[i]);
    }
    
    new_buffer[size - 1].right = free_ptr;
    free_ptr = &(new_buffer[0]);

    return true;
  }

  bool init(const size_t treemap_size) {
    return !size() && treemap_size ? grow(treemap_size) : false;
  }

  #ifdef ARENA_POOL_CPP
  bool init(apc::arena& _arena, const size_t treemap_size) {
    if(size() || !treemap_size) return false;

    arena = &_arena;

    if(!grow(treemap_size)) {
      arena = nullptr;
      return false;
    }

    return true;
  }
  #endif

  APC_TREEMAP_COMMON

#ifdef APC_TREEMAP_TESTING 
  treemap_node<T, K>* get_root() { return root; }
#endif
};

}
