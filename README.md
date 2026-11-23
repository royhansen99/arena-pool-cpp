# arena-pool-cpp

An arena-style bump-allocator with a suite of containers that  
can use arenas as the underlying allocator. All containers  
also feature fixed-size stack or dynamic malloc-allocation,  
outside of arenas. This library aims to have as little overhead  
as possible, by avoiding dynamic-dispatch and virtual-lookups.

It should be much faster than the standard library std::pmr  
suite of containers, which has virtual-lookup overhead  
(virtual methods). Benchmarks indicate that it is on-par with  
the speed of std::vector, when using malloc instead of arenas.  

This library targets C++11.

Containers supported:
- Arenas  
- Strings  
- Vectors  
- Pools  
- Hashmaps  
- Treemaps

All containers are optimized for trivial structures (POD),  
and will use memcpy and avoid calling destructors when possible.

All containers are RAII-compatible with automatic cleanup and  
copy/move-semantics.

All dynamic containers support paging and will grow  
automatically, including arenas themselves.  
Fixed-size stack-containers will not grow beyond their starting  
size.

Arenas, Hashmaps, Treemaps and Pools will maintain stable  
pointers as they grow.

In general, all containers will double size when they are full.  

All containers allow you to manually/forcefully specify that your  
data-structure is trivially copyable in the template-signature.

Container methods are in some cases very similar to  
std::vector/string/.., but not in all cases.

Nested arenas (child-arenas) are supported.

All containers support iterators for convenient for-loop iteration.

In order to get maximum speed you should attempt to make your  
data-structures trivial-pod, since that makes your data faster  
to move around and destroy in containers.

### Motivation 

This library was primarily designed as a faster bump-allocator  
alternative to std::pmr, without virtual-lookup overhead.

The reason to use a bump allocator is that you can malloc a big  
chunk of memory once, and then simply re-use that chunk for all  
containers. In-theory all your dynamic memory could be boiled  
down to a single arena/malloc, which is huge.

In addition all containers also feature fixed-size (stack)  
variations and dynamic-size (malloc) variations.  
So you can use these containers without using arenas at all, which  
allows you to simply blanket replace all std-vector/string..  
container-usage.

### Containers overview 

| Container | Details |
| --- | --- |
|apc::arena|- Bump allocator.<br />- Can be dynamic with automatic growth, or static with a fixed size (e.g stack allocation).|
|apc::pool<T><br />apc::pool\_fixed<T, size>|- Basically a doubly linked-list.<br />- Stable pointers.|
|apc::vector<T><br />apc::vector\_fixed<T, size>|- An array/vector container, very similar to std::vector.<br />- Also features static-size vectors which can be stack allocated.|
|apc::hashmap<T><br />apc::hashmap\_fixed<T, size>|- Uses Rapidhash for hashing keys.<br />- Hashmap keys can even be structs, as long as you zero-out padding.<br />- Stable pointers.<br />- Data is stored in a doubly linked-list, and hash-keys are stored in an array which simply points back to entries in the linked-list.<br />- Automatically resizes/recalculates all keys/array if more than 75% is used.|
|apc::treemap<T, K><br />apc::treemap\_fixed<T, K, size>|- Red/black binary tree.<br />- Supports specifying custom sorting/comparator.<br />- Stable pointers.<br />- Implemented by following logic in the book "Introduction to Algorithms 4th edition (CLRS)".<br />- Nodes are stored in a doubly linked-list.|
|apc::str<br />apc::str\_fixed\<size\><br />apc::str4<br />apc::str8<br />apc::str16<br />apc::str32<br />apc::str64<br />apc::str128<br />apc::str256|- String containers, very similar to std::string.<br />- Also features static-size strings which can be stack allocated.<br />- Has SSO (small-string-optimization), dynamic strings smaller than 32 bytes uses stack.|


### Examples

For usage examples please look at tests in the tests/ folder.

### Benchmarks

__Smaller numbers is better!__

```
Benchmarking 10M int allocations with a single cheap reset
apc::arena              alloc:   0.62 ns  reset:   0.00 ns
apc::pool (reserve)     alloc:   2.44 ns  reset:   0.80 ns
apc::pool (dynamic)     alloc:   5.43 ns  reset:   1.44 ns
apc::vector (reserve)   alloc:   0.74 ns  reset:   0.00 ns
apc::vector (dynamic)   alloc:   0.99 ns  reset:   0.00 ns
apc::hashmap (reserve)  alloc:  54.18 ns  reset:   0.79 ns
apc::hashmap (dynamic)  alloc:  63.18 ns  reset:   0.94 ns
apc::treemap (reserve)  alloc:   7.96 ns  reset:   0.73 ns
apc::treemap (dynamic)  alloc:  13.84 ns  reset:   3.00 ns
pmr::vector (reserve)   alloc:   0.69 ns  reset:   0.00 ns
pmr::vector (dynamic)   alloc:   1.30 ns  reset:   0.00 ns
std::vector (reserve)   alloc:   0.50 ns  reset:   0.00 ns
std::vector (dynamic)   alloc:   0.97 ns  reset:   0.00 ns
std::list (dynamic)     alloc:  10.15 ns  reset:  12.10 ns
std::map (dynamic)      alloc:  52.66 ns  reset:  16.70 ns
```

```
Benchmarking 100K int allocations with individual expensive erase
apc::arena              (individual reset not supported)
apc::pool (reserve)     alloc:   1.13 ns  reset:   0.39 ns
apc::pool (dynamic)     alloc:   3.86 ns  reset:   0.52 ns
apc::vector (reserve)   alloc:   0.69 ns  reset: 3390.79 ns
apc::vector (dynamic)   alloc:   1.23 ns  reset: 3408.72 ns
apc::hashmap (reserve)  alloc:  18.58 ns  reset:  16.70 ns
apc::hashmap (dynamic)  alloc:  23.33 ns  reset:  13.50 ns
apc::treemap (reserve)  alloc:   7.18 ns  reset:  17.52 ns
apc::treemap (dynamic)  alloc:  10.61 ns  reset:  18.70 ns
pmr::vector (reserve)   alloc:   0.75 ns  reset: 3362.82 ns
pmr::vector (dynamic)   alloc:   1.89 ns  reset: 3361.03 ns
std::vector (reserve)   alloc:   0.77 ns  reset: 3364.47 ns
std::vector (dynamic)   alloc:   0.70 ns  reset: 3331.71 ns
std::list (dynamic)     alloc:  10.61 ns  reset:  11.64 ns
std::map (dynamic)      alloc:  35.69 ns  reset:  21.59 ns
```

### Build tests 

// MacOS
> cmake -S . -B build -G Xcode

// Windows
> cmake -S . -B build -G "Visual Studio 17 2022"

Open the appropriate project-file in Xcode or visual studio, choose the target executable to build/run.
