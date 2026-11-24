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
|apc::hashmap<T><br />apc::hashmap\_fixed<T, size><br />apc::hashmap_str<T, STRING_KEY_SIZE><br />apc::hashmap_fixed_str<T, STRING_KEY_SIZE, size>|- Uses Rapidhash for hashing keys.<br />- Hashmap keys can even be structs, as long as you zero-out padding.<br />- Stable pointers.<br />- Data is stored in a doubly linked-list, and hash-keys are stored in an array which simply points back to entries in the linked-list.<br />- Automatically resizes/recalculates all keys/array if more than 75% is used.|
|apc::treemap<T, K><br />apc::treemap\_fixed<T, K, size>|- Red/black binary tree.<br />- Supports specifying custom sorting/comparator.<br />- Stable pointers.<br />- Implemented by following logic in the book "Introduction to Algorithms 4th edition (CLRS)".<br />- Nodes are stored in a doubly linked-list.|
|apc::str<br />apc::str\_fixed\<size\><br />apc::str4<br />apc::str8<br />apc::str16<br />apc::str32<br />apc::str64<br />apc::str128<br />apc::str256|- String containers, very similar to std::string.<br />- Also features static-size strings which can be stack allocated.<br />- Has SSO (small-string-optimization), dynamic strings smaller than 32 bytes uses stack.|


### Examples

For usage examples please look at tests in the tests/ folder.

### Benchmarks

__Smaller numbers is better!__

```
Benchmarking 10M int allocations with a single cheap reset
apc::arena              insert:   0.72 ns  reset:   0.00 ns
apc::pool (reserve)     insert:   1.83 ns  reset:   0.69 ns
apc::pool (dynamic)     insert:   5.07 ns  reset:   1.21 ns
apc::vector (reserve)   insert:   0.73 ns  reset:   0.00 ns
apc::vector (dynamic)   insert:   0.89 ns  reset:   0.00 ns
apc::hashmap (reserve)  insert:  52.82 ns  reset:   0.78 ns
apc::hashmap (dynamic)  insert:  62.04 ns  reset:   0.97 ns
apc::treemap (reserve)  insert:   8.28 ns  reset:   0.80 ns
apc::treemap (dynamic)  insert:  13.32 ns  reset:   1.71 ns
pmr::vector (reserve)   insert:   0.72 ns  reset:   0.00 ns
pmr::vector (dynamic)   insert:   1.32 ns  reset:   0.00 ns
std::vector (reserve)   insert:   0.51 ns  reset:   0.00 ns
std::vector (dynamic)   insert:   0.98 ns  reset:   0.00 ns
std::list (dynamic)     insert:  10.33 ns  reset:  10.70 ns
std::map (dynamic)      insert:  52.76 ns  reset:  16.96 ns
```


```
Benchmarking 100K int allocations with individual expensive erase
apc::arena              (individual erase not supported)
apc::pool (reserve)     insert:   1.15 ns  erase:   0.44 ns
apc::pool (dynamic)     insert:   3.65 ns  erase:   0.53 ns
apc::vector (reserve)   insert:   0.70 ns  erase: 3343.67 ns
apc::vector (dynamic)   insert:   1.22 ns  erase: 3346.95 ns
apc::hashmap (reserve)  insert:  14.73 ns  erase:  15.71 ns
apc::hashmap (dynamic)  insert:  21.56 ns  erase:  12.13 ns
apc::treemap (reserve)  insert:   7.45 ns  erase:  17.34 ns
apc::treemap (dynamic)  insert:   9.64 ns  erase:  18.79 ns
pmr::vector (reserve)   insert:   0.73 ns  erase: 3350.74 ns
pmr::vector (dynamic)   insert:   1.82 ns  erase: 3354.85 ns
std::vector (reserve)   insert:   0.72 ns  erase: 3343.66 ns
std::vector (dynamic)   insert:   0.79 ns  erase: 3345.92 ns
std::list (dynamic)     insert:   9.24 ns  erase:  11.14 ns
std::map (dynamic)      insert:  32.15 ns  erase:  19.84 ns
```


```
Benchmarking 3M lookups/find across 10M int keys
apc::pool    (too slow)
apc::vector  (too slow)
apc::hashmap find:   1.83 ns
apc::treemap find:   6.22 ns
std::vector  (too slow)
std::list    (too slow)
std::map     find:   3.35 ns
```

### Build tests 

// MacOS
> cmake -S . -B build -G Xcode

// Windows
> cmake -S . -B build -G "Visual Studio 17 2022"

Open the appropriate project-file in Xcode or visual studio, choose the target executable to build/run.
