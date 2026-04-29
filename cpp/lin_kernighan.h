#ifndef LIN_KERNIGHAN_H
#define LIN_KERNIGHAN_H

#include <vector>
#include <utility>

using DistMatrix = std::vector<std::vector<int>>;

/**
 * Lin-Kernighan heuristic for TSP
 * Returns the cost of a good tour (not optimal, used for upper bound in branch-and-bound)
 */
long long lin_kernighan_tsp(const DistMatrix& dist, 
                            int max_iterations = 1000,
                            int random_restarts = 1);

/**
 * Lin-Kernighan heuristic for TSP with tour return
 * Returns both the best tour found and its cost
 */
std::pair<std::vector<int>, long long> lin_kernighan_tsp_with_tour(const DistMatrix& dist,
                                                                    int max_iterations = 1000,
                                                                    int random_restarts = 1);

#endif
