// COMPILE: g++ -std=c++17 -O3 -march=native benchmarks_alloc.cpp 

#include "../src/arena.h"
#include "../src/pool.h"
#include "../src/vector.h"
#include "../src/hashmap.h"
#include "../src/treemap.h"
#include <memory_resource>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <list>
#include <map>

using Clock = std::chrono::high_resolution_clock;
using ns = std::chrono::nanoseconds;

struct Benchmark {
  char description[200];
  bool single_free;
  size_t N;
};

int main() {
    Benchmark b[2];

    strncpy(b[0].description, "Benchmarking 10M int allocations with a single cheap reset\n", 200);
    b[0].single_free = true;
    b[0].N = 10000000;   // 10 million

    strncpy(b[1].description, "Benchmarking 100K int allocations with individual expensive erase\n", 200);
    b[1].single_free = false;
    b[1].N = 100000;   // 100k

    std::cout << std::fixed << std::setprecision(2);

    for(int i = 0; i < (sizeof(b) / sizeof(Benchmark)); i++) { 
      Benchmark& benchmark = b[i];
      bool& single_free = benchmark.single_free;
      size_t& N = benchmark.N;
      const size_t CAP = N;   // 1 million per buffer

      printf(benchmark.description, N);

      // --------------------------------------------------------------
      // apc::arena 
      // --------------------------------------------------------------
      if(single_free) { // Only run this benchmark for single frees
        apc::arena arena(1024ULL * 1024 * 1024);  // 1 GiB
        int* p = static_cast<int*>(arena.allocate_raw(N, alignof(int)));
        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i) {
          p[i] = static_cast<int>(i);
        }
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        arena.reset();
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::arena              insert: " << std::setw(6) << alloc_ns
                  << " ns  reset: " << std::setw(6) << dealloc_ns << " ns\n";
      } else {
        std::cout << "apc::arena              (individual erase not supported)\n";
      }

      // --------------------------------------------------------------
      // apc::pool (reserve)
      // --------------------------------------------------------------
      {
        apc::pool<int> pool(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i) {
          int* p = pool.allocate_new(static_cast<int>(i));
        }
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        pool.reset(); // This is essentially individual frees, since
                      // the pool must cycle through every item in
                      // the pool.
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::pool (reserve)     insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::pool (dynamic)
      // --------------------------------------------------------------
      {
        apc::pool<int> pool(1);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i) {
          int* p = pool.allocate_new(static_cast<int>(i));
        }
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        pool.reset(); // This is essentially individual frees, since
                      // the pool must cycle through every item in
                      // the pool.
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::pool (dynamic)     insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::vector (reserve) 
      // --------------------------------------------------------------
      {
        apc::vector<int> arr(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push(i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          arr.reset();
        } else {
          while (!arr.empty()) {
            arr.erase_ptr(arr.first());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::vector (reserve)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::vector (dynamic) 
      // --------------------------------------------------------------
      {
        apc::vector<int> arr;

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push(i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          arr.reset();
        } else {
          while (!arr.empty()) {
            arr.erase_ptr(arr.first());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::vector (dynamic)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::hashmap (reserve) 
      // --------------------------------------------------------------
      {
        apc::hashmap<int, sizeof(int)> map(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          map.insert(&i, i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          map.reset();
        } else {
          while (map.begin() != nullptr) {
            map.erase(map.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::hashmap (reserve)  insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::hashmap (dynamic) 
      // --------------------------------------------------------------
      {
        apc::hashmap<int, sizeof(int)> map;

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          map.insert(&i, i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          map.reset();
        } else {
          while (map.begin() != nullptr) {
            map.erase(map.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::hashmap (dynamic)  insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::treemap (reserve) 
      // --------------------------------------------------------------
      {
        apc::treemap<int, int> map(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          map.insert(i, i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          map.reset();
        } else {
          while (map.begin() != nullptr) {
            map.erase(map.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::treemap (reserve)  insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::treemap (dynamic) 
      // --------------------------------------------------------------
      {
        apc::treemap<int, int> map;

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          map.insert(i, i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          map.reset();
        } else {
          while (map.begin() != nullptr) {
            map.erase(map.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::treemap (dynamic)  insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // pmr::vector (reserve) 
      // --------------------------------------------------------------
      {
        std::pmr::monotonic_buffer_resource pool(1024ULL * 1024 * 1024);

        std::pmr::vector<int> arr(&pool);
        arr.reserve(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push_back(i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          arr.clear();
        } else {
          while (!arr.empty()) {
            arr.erase(arr.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "pmr::vector (reserve)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // pmr::vector (dynamic) 
      // --------------------------------------------------------------
      {
        std::pmr::monotonic_buffer_resource pool(1024ULL * 1024 * 1024);
        std::pmr::vector<int> arr(&pool);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push_back(i);
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          arr.clear();
        } else {
          while (!arr.empty()) {
            arr.erase(arr.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "pmr::vector (dynamic)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::vector (reserve())
      // --------------------------------------------------------------
      {
        std::vector<int> vec;
        vec.reserve(N);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          vec.emplace_back(static_cast<int>(i));
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          vec.clear();  // REAL deallocation: just set size = 0
        } else {
          while (!vec.empty()) {
            vec.erase(vec.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::vector (reserve)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::vector (dynamic)
      // --------------------------------------------------------------
      {
        std::vector<int> vec;

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          vec.emplace_back(static_cast<int>(i));
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          vec.clear();
        } else {
          while (!vec.empty()) {
            vec.erase(vec.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::vector (dynamic)   insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::list (dynamic)
      // --------------------------------------------------------------
      {
        std::list<int> list; 

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          list.push_back(static_cast<int>(i));
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          list.clear();
        } else {
          while (!list.empty()) {
            list.pop_back();
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::list (dynamic)     insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::map (dynamic)
      // --------------------------------------------------------------
      {
        std::map<int, int> map; 

        auto t0 = Clock::now();
        for (int i = 0; i < N; ++i)
          map.emplace(i, i);

        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        if(single_free) {
          map.clear();
        } else {
          while (!map.empty()) {
            map.erase(map.begin());
          }
        }
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::map (dynamic)      insert: " << std::setw(6) << alloc_ns
                  << (!single_free ? " ns  erase: " : " ns  reset: ") << std::setw(6) << dealloc_ns << " ns\n";
      }

      std::cout << "\n\n";
    }

    return 0;
}
