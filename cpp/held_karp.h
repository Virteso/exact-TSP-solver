#ifndef HELD_KARP_H
#define HELD_KARP_H

#include <vector>
#include <unordered_map>

using DistMatrix = std::vector<std::vector<int>>;

/**
 * Held-Karp algorithm for TSP (dynamic programming)
 * Time: O(n^2 * 2^n), Space: O(n * 2^n)
 */
int held_karp(const DistMatrix& dist_matrix, bool verbose = false);

#endif
