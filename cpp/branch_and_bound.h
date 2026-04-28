#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

#include <vector>

using DistMatrix = std::vector<std::vector<int>>;

/**
 * Branch and Bound algorithm for TSP
 * Uses lower bound estimation to prune the search tree
 */
long long branch_and_bound_tsp(const DistMatrix& dist_matrix, bool verbose = false);

#endif
