#include "brute_force.h"
#include <algorithm>
#include <iostream>
#include <climits>

long long brute_force_tsp(const DistMatrix& dist, bool verbose) {
    int n = dist.size();
    if (n == 0) return 0;
    if (n == 1) return 0;
    
    // Create a vector of nodes excluding the first node
    std::vector<int> nodes;
    for (int i = 1; i < n; i++) {
        nodes.push_back(i);
    }
    
    long long best_cost = LLONG_MAX;
    long long count = 0;
    
    // Try all permutations
    std::sort(nodes.begin(), nodes.end());
    do {
        // Calculate cost for this permutation
        // Start from node 0, visit all nodes in permutation order, return to 0
        long long cost = dist[0][nodes[0]];
        for (int i = 0; i < (int)nodes.size() - 1; i++) {
            cost += dist[nodes[i]][nodes[i+1]];
        }
        cost += dist[nodes.back()][0];
        
        if (cost < best_cost) {
            best_cost = cost;
            if (verbose) {
                std::cout << "Found better solution: " << best_cost << std::endl;
            }
        }
        
        count++;
        if (verbose && count % 100000 == 0) {
            std::cout << "Checked " << count << " permutations, best so far: " << best_cost << std::endl;
        }
    } while (std::next_permutation(nodes.begin(), nodes.end()));
    
    return best_cost;
}
