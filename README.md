Sux 1.0.3
=========

Welcome to the C++ part of the [Sux](http://sux.di.unimi.it/) project.

Available classes
-----------------

The classes we provide fall into three categories:

* Static structures for ranking and selection based on the paper
  ["Broadword Implementation of Rank/Select
  Queries"](http://vigna.di.unimi.it/papers.php#VigBIRSQ). We provide also
  an implementation of the Elias-Fano representation of monotone sequences
  that can be used as an opportunistic bitvector representation.

* Fenwick trees with bounded leaf size, and associated dynamic structures
  for ranking and selection based on the paper ["Compact Fenwick Trees for
  Dynamic Ranking and
  Selection"](http://vigna.di.unimi.it/papers.php#MaVCFTDRS) (with Stefano
  Marchini).

* Minimal perfect hashing functions based on the paper ["RecSplit: Minimal
  Perfect Hashing via Recursive
  Splitting"](http://vigna.di.unimi.it/papers.php#EGVRS) (with Emmanuel
  Esposito and Thomas Mueller Graf).

All classes are heavily asserted. For testing speed, remember to use `-DNDEBUG`.

Documentation can be generated by running `doxygen`.

All provided classes are templates, so you just have to copy the files in
the `sux` directory somewhere in your include path.

Benchmarks
----------

The commands `make ranksel`, `make recsplit`, `make fenwick` and `make
dynranksek` will generate binaries in `bin` with which you can test the
speed of RecSplit,  rank/select static structures, compact Fenwick trees
and dynamic rank/select structures. Note that you can set the `make`
variable `LEAF` to change the leaf size of RecSplit, as in `make recsplit
LEAF=4`, and the variable `ALLOC_TYPE` to the possible values of 
sux::util::AllocType to experiment, for example, with transparent huge
pages on Linux.

For ranking and selection, we generate one binary for each type of structure,
with some variation on parameters (see the makefile for more details). Beside
the number of bits, you can provide one or two probabilities. Bits will be set
to one with the given probability in the first half of the test array, and with
the second probability in the second half (if no second probability is
specified, it is assumed to be equal to the first one). This setup is necessary
to show the slow behaviour of naive implementations on
half-almost-empty-half-almost-full arrays.

For RecSplit, we provide dump/load binaries which dump on disk a minimal
perfect hash function, and test it. The standard version uses a keys file for
the keys, whereas the “128” version uses 128-bit random keys. We suggest the
latter for benchmarking as in any case the first step in RecSplit construction
is mapping to 128-bit hashes.

Testing
-------
Requirements: CMake >= 3.16.2

### Build
```
mkdir bin
cd bin
cmake -DSUX_TESTING=1 ..
cmake --build .
```

### Unit Tests
```
./test/bits_unittest --gtest_color=yes
./test/function_unittest --gtest_color=yes
./test/util_unittest --gtest_color=yes
```

Licensing
---------

Sux is licensed exactly like `libstdc++` (GPLv3 + GCC Runtime Library
Exception), which essentially means you can use it everywhere, exactly
like `libstdc++`.

seba (<mailto:sebastiano.vigna@unimi.it>)
