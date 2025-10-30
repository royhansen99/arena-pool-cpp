// COMPILE: g++ -std=c++11 -O3 -march=native -DNDEBUG benchmarks_alloc.cpp 

#include "./Arena.h"
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>

using Clock = std::chrono::high_resolution_clock;
using ns = std::chrono::nanoseconds;

int main() {
    const size_t N = 10000000;   // 10 million
    const size_t CAP = N / 10;   // 1 million per buffer

    std::cout << "Benchmarking " << N << " int allocations...\n";
    std::cout << std::fixed << std::setprecision(2);

    // --------------------------------------------------------------
    // 1. Arena
    // --------------------------------------------------------------
    {
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

        std::cout << "Arena                 alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
    }

    // --------------------------------------------------------------
    // 2. Pool (using Arena)
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
        pool.reset();
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "Pool (Arena)          alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
    }

    // --------------------------------------------------------------
    // 3. Pool (using malloc)
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
        pool.reset();
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "Pool (malloc)         alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
    }

    // --------------------------------------------------------------
    // 4. std::vector (fixed capacity)
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
        vec.clear();  // REAL deallocation: just set size = 0
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::vector (fixed)   alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
    }

    // --------------------------------------------------------------
    // 5. std::vector (dynamic)
    // --------------------------------------------------------------
    {
        std::vector<int> vec;

        auto t0 = Clock::now();
        for (size_t i = 0; i < N; ++i)
            vec.emplace_back(static_cast<int>(i));
        auto t1 = Clock::now();
        double alloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        t0 = Clock::now();
        vec.clear();
        t1 = Clock::now();
        double dealloc_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::vector (dynamic) alloc: " << std::setw(6) << alloc_ns
                  << " ns  dealloc: " << std::setw(6) << dealloc_ns << " ns\n";
    }

    return 0;
}
