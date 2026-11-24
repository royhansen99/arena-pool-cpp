// COMPILE: g++ -std=c++17 -O3 -march=native tests/benchmarks_find.cpp 

#include "../src/pool.h"
#include "../src/vector.h"
#include "../src/hashmap.h"
#include "../src/treemap.h"
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
    Benchmark b[1];

    int ranges[30][2] = {
      {7234000, 7334000},
      {102000, 202000},
      {891000, 991000},
      {4567000, 4667000},
      {290000, 390000},
      {9342000, 9442000},
      {591000, 691000},
      {8125000, 8225000},
      {450000, 550000},
      {6789000, 6889000},
      {130000, 230000},
      {5432000, 5532000},
      {10000, 110000},
      {9678000, 9778000},
      {190000, 290000},
      {3526000, 3626000},
      {427000, 527000},
      {7851000, 7951000},
      {948000, 1048000},
      {2345000, 2445000},
      {6213000, 6313000},
      {1678000, 1778000},
      {9981000, 10000000},
      {3894000, 3994000},
      {856000, 956000},
      {5127000, 5227000},
      {7412000, 7512000},
      {2986000, 3086000},
      {9054000, 9154000},
      {4768000, 4868000}
    };

    strncpy(b[0].description, "Benchmarking 3M lookups/find across 10M int keys\n", 200);
    b[0].single_free = false;
    b[0].N = 10000000;  // 10M

    std::cout << std::fixed << std::setprecision(2);

    for(int i = 0; i < (sizeof(b) / sizeof(Benchmark)); i++) { 
      Benchmark& benchmark = b[i];
      bool& single_free = benchmark.single_free;
      size_t& N = benchmark.N;
      const size_t CAP = N;   // 1 million per buffer

      printf(benchmark.description, N);

      // --------------------------------------------------------------
      // apc::pool
      // --------------------------------------------------------------
      {
        apc::pool<int> pool(CAP);

        for (size_t i = 0; i < N; ++i)
          int* p = pool.allocate_new(static_cast<int>(i));

        auto t0 = Clock::now();


        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        // std::cout << "apc::pool    find: " << std::setw(6) << find_ns << " ns\n";
        std::cout << "apc::pool    (too slow)\n";
      }

      // --------------------------------------------------------------
      // apc::vector
      // --------------------------------------------------------------
      {
        apc::vector<int> arr(CAP);

        for (size_t i = 0; i < N; ++i)
          arr.push(i);

        auto t0 = Clock::now();

        for(int i = 0; i < 30; i++) {
          for(int x = ranges[i][0]; x < ranges[i][1]; x ++) {
            // auto it = std::find(arr.begin(), arr.end(), x);
            // if(*it != x) std::cout << "ERROR!\n";
          }
        }

        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        // std::cout << "apc::vector  find: " << std::setw(6) << find_ns << " ns\n";
        std::cout << "apc::vector  (too slow)\n";
      }

      // --------------------------------------------------------------
      // apc::hashmap
      // --------------------------------------------------------------
      {
        apc::hashmap<int, sizeof(int)> map(CAP);

        for (size_t i = 0; i < N; ++i)
          map.insert(&i, i);

        auto t0 = Clock::now();

        for(int i = 0; i < 30; i++) {
          for(int x = ranges[i][0]; x < ranges[i][1]; x ++) {
            if(*map.find(&x) != x) std::cout << "ERROR!\n";
          }
        }

        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::hashmap find: " << std::setw(6) << find_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // apc::treemap
      // --------------------------------------------------------------
      {
        apc::treemap<int, int> map(CAP);

        for (size_t i = 0; i < N; ++i)
          map.insert(i, i);

        auto t0 = Clock::now();

        for(int i = 0; i < 30; i++) {
          for(int x = ranges[i][0]; x < ranges[i][1]; x ++) {
            if(*map.find(x) != x) std::cout << "ERROR!\n";
          }
        }

        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "apc::treemap find: " << std::setw(6) << find_ns << " ns\n";
      }

      // --------------------------------------------------------------
      // std::vector
      // --------------------------------------------------------------
      {
        std::vector<int> vec;
        vec.reserve(N);

        for (size_t i = 0; i < N; ++i)
          vec.emplace_back(static_cast<int>(i));

        auto t0 = Clock::now();
        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        // std::cout << "std::vector  find: " << std::setw(6) << find_ns << " ns\n";
        std::cout << "std::vector  (too slow)\n";
      }

      // --------------------------------------------------------------
      // std::list
      // --------------------------------------------------------------
      {
        std::list<int> list; 

        for (size_t i = 0; i < N; ++i)
          list.push_back(static_cast<int>(i));

        auto t0 = Clock::now();

        for(int i = 0; i < 30; i++) {
          for(int x = ranges[i][0]; x < ranges[i][1]; x ++) {
            // auto it = std::find(list.begin(), list.end(), x);
            // if(*it != x) std::cout << "ERROR!\n";
          }
        }

        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        // std::cout << "std::list    find: " << std::setw(6) << find_ns << " ns\n";
        std::cout << "std::list    (too slow)\n";
      }

      // --------------------------------------------------------------
      // std::map
      // --------------------------------------------------------------
      {
        std::map<int, int> map; 

        for (int i = 0; i < N; ++i)
          map.emplace(i, i);

        auto t0 = Clock::now();

        for(int i = 0; i < 30; i++) {
          for(int x = ranges[i][0]; x < ranges[i][1]; x ++) {
            if(map.find(x)->second != x) std::cout << "ERROR!\n";
          }
        }

        auto t1 = Clock::now();
        double find_ns = std::chrono::duration_cast<ns>(t1 - t0).count() / double(N);

        std::cout << "std::map     find: " << std::setw(6) << find_ns << " ns\n";
      }

      std::cout << "\n\n";
    }

    return 0;
}
