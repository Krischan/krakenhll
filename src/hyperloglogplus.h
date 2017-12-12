/*
 * Copyright 2017, Florian Breitwieser
 *
 * This file is part of the KrakenHLL taxonomic sequence classification system.
 *
 * KrakenHLL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KrakenHLL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kraken.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Implementation of 64-bit HyperLogLog algorithm by Flajolet et al., 
 *   with sparse mode for increased precision with low cardinalities (Stefan Heule et al.), and 
 *   an improved estimator that does not rely on empirical bias correction data (Otmar Ertl)
 */

#ifndef HYPERLOGLOGPLUS_H_
#define HYPERLOGLOGPLUS_H_

#include<vector>
#include<stdexcept>
#include<iostream>
#include<fstream>
#include<math.h>    //log
#include<algorithm> //vector.count
#include<bitset>
#include<unordered_set>

#include "hyperloglogbias.h"
#include "assert_helpers.h"

using namespace std;

//#define HLL_DEBUG
//#define HLL_DEBUG2

#ifdef HLL_DEBUG 
#define D(x) x
#else 
#define D(x)
#endif

// 64-bit Mixer/finalizer from MurMurHash3 https://github.com/aappleby/smhasher
uint64_t murmurhash3_finalizer (uint64_t key);


// Heule et al. encode the sparse list with variable length encoding
//   see section 5.3.2. This implementation just uses a sorted vector or unordered_set.
typedef unordered_set<uint32_t> SparseListType;
// Other possible SparseList types:
// // typedef vector<uint32_t> SparseListType;
// The sorted vector implementation is pretty inefficient currently, as the vector
// is always kept sorted. 
// //typedef unordered_set<uint32_t, NoHash<uint32_t> > SparseListType;
// No real performance gain from using the hash value directly - and probably problems
// with the bucket assignment, since unordered_set expects size_t hashes

/**
 * HyperLogLogPlusMinus class for counting the number of unique 64-bit values in stream
 * Note that only HASH=uint64_t is implemented.
 */
template<typename HASH>
class HyperLogLogPlusMinus {

private:
  uint8_t p;      // precision, set in constructor
  size_t m = 1 << p;  // number of registers

  vector<uint8_t> M;    // registers, size m
  bool sparse;          // sparse representation of the data?
  SparseListType sparseList;
  HASH (*bit_mixer) (uint64_t);

  // sparse versions of p and m
  static const uint8_t  pPrime = 25; // precision when using a sparse representation
                                     // fixed to 25, because 25 + 6 bits for rank + 1 flag bit = 32
  static const uint32_t mPrime = 1 << pPrime; // 2^pPrime

public:

  // Construct HLL with precision bits
  HyperLogLogPlusMinus(uint8_t precision=12, bool sparse=true, HASH (*bit_mixer) (uint64_t) = murmurhash3_finalizer);
  ~HyperLogLogPlusMinus() {};
  void reset(); // Note: sets sparse=true

  // Add items or other HLL to this sketch
  void add(uint64_t item);
  void add(vector<uint64_t> items);
  void add(const HyperLogLogPlusMinus<HASH>* other);
  HyperLogLogPlusMinus<HASH>& operator+=(const HyperLogLogPlusMinus<HASH>* other);
  HyperLogLogPlusMinus<HASH>& operator+=(const HyperLogLogPlusMinus<HASH>& other);

  // Calculate cardinality estimates
  uint64_t cardinality() const; // returns heuleCardinality()
  // HLL++ estimator of Heule et al., 2015. Uses empirical bias correction factors
  uint64_t heuleCardinality() const; 
  // Improved estimator of Ertl, 2017. Does not rely on empirical data
  uint64_t ertlCardinality() const;

private:
  void switchToNormalRepresentation();
  void addToRegisters(const SparseListType &sparseList);

};

#endif /* HYPERLOGLOGPLUS_H_ */
