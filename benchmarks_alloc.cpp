// COMPILE: g++ -std=c++11 -O3 -march=native -DNDEBUG benchmarks_alloc.cpp 

#include "./Arena.h"
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <list>

using Clock = std::chrono::high_resolution_clock;
using ns = std::chrono::nanoseconds;

struct Benchmark {
  char description[200];
  bool single_free;
  size_t N;
};

int main() {
    Benchmark b[2];

    strncpy(b[0].description, "Benchmarking %ld int allocations with a single cheap mass-dealloc/reset\n", 200);
    b[0].single_free = true;
    b[0].N = 10000000;   // 10 million

    strncpy(b[1].description, "Benchmarking %ld int allocations with individual expensive dealloc\n", 200);
    b[1].single_free = false;
    b[1].N = 100000;   // 10 million

    std::cout << std::fixed << std::setprecision(2);

    for(int i = 0; i < (sizeof(b) / sizeof(Benchmark)); i++) { 
      Benchmark& benchmark = b[i];
      bool& single_free = benchmark.single_free;
      size_t& N = benchmark.N;
      const size_t CAP = N / 10;   // 1 million per buffer

      printf(benchmark.description, N);

      // --------------------------------------------------------------
      // Arena
      // --------------------------------------------------------------
      if(single_free) { // Only run this benchmark for single frees
        Arena arena(1024ULL * 1024 * 1024);  // 1 GiB
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

        std::cout << "Arena                   alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
        } else {
        std::cout << "Arena                   (individual dealloc not supported)\n";
      }

      // --------------------------------------------------------------
      // Pool (Arena)
      // --------------------------------------------------------------
      {
        Arena arena(1024ULL * 1024 * 1024);  // 1 GiB
        Pool<int> pool(arena, CAP);

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

        std::cout << "Pool (Arena)            alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // Pool (malloc)
      // --------------------------------------------------------------
      {
        Pool<int> pool(CAP);

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

        std::cout << "Pool (malloc)           alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // SArray (arena) 
      // --------------------------------------------------------------
      {
        Arena a((sizeof(int) * CAP) + (sizeof(bool) * CAP));
        SArray<int> arr(a, CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push_new(i);
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

        std::cout << "SArray (arena)          alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // SArray (malloc) 
      // --------------------------------------------------------------
      {
        SArray<int> arr(CAP);

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
          arr.push_new(i);
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

        std::cout << "SArray (malloc)         alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
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

        std::cout << "std::vector (reserve()) alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
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

        std::cout << "std::vector (dynamic)   alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::list
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

        std::cout << "std::list               alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
      }

      std::cout << "\n\n";
    }

    return 0;
}
