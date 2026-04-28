#ifndef BRUTE_FORCE_H
#define BRUTE_FORCE_H

#include <vector>

using DistMatrix = std::vector<std::vector<int>>;

// Brute force TSP solver - tries all permutations
// Time complexity: O(n! * n)
long long brute_force_tsp(const DistMatrix& dist, bool verbose = false);

#endif // BRUTE_FORCE_H
