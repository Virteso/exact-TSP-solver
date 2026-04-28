#ifndef ONE_TREE_H
#define ONE_TREE_H

#include <vector>

using DistMatrix = std::vector<std::vector<int>>;

/**
 * Compute Minimum Spanning Tree using Prim's algorithm
 * Optionally excludes a specified node
 */
std::pair<long long, std::vector<std::pair<int, int>>> 
compute_mst_weight(const DistMatrix& dist, int excluded_node = -1);

/**
 * Compute 1-tree lower bound for TSP
 * A 1-tree is MST of nodes {1, 2, ..., n-1} plus the two minimum edges from node 0
 */
long long compute_one_tree_bound(const DistMatrix& dist);

#endif
