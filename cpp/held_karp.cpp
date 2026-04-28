#include "held_karp.h"
#include <algorithm>
#include <climits>
#include <vector>

int held_karp(const DistMatrix& dist, bool verbose) {
    int n = dist.size();
    if (n <= 1) return 0;
    if (n == 2) return dist[0][1] + dist[1][0];
    
    // dp[mask][k] = minimum cost to visit all nodes in mask ending at node k
    // mask represents a subset of nodes {1, 2, ..., n-1}
    std::vector<std::vector<long long>> dp(1 << (n - 1), 
                                           std::vector<long long>(n, LLONG_MAX));
    
    // Base case: single nodes
    for (int k = 1; k < n; k++) {
        int mask = 1 << (k - 1);
        dp[mask][k] = dist[0][k];
    }
    
    // Build subsets by size
    for (int subset_size = 2; subset_size < n; subset_size++) {
        // Iterate through all masks with exactly subset_size bits set
        for (int mask = 1; mask < (1 << (n - 1)); mask++) {
            if (__builtin_popcount(mask) != subset_size) continue;
            
            // For each node k in this mask
            for (int k = 1; k < n; k++) {
                if (!(mask & (1 << (k - 1)))) continue;  // k not in mask
                
                int prev_mask = mask ^ (1 << (k - 1));
                
                // Try all possible previous nodes m in prev_mask
                long long best = LLONG_MAX;
                for (int m = 1; m < n; m++) {
                    if (m == k || !(prev_mask & (1 << (m - 1)))) continue;
                    
                    if (dp[prev_mask][m] != LLONG_MAX) {
                        long long cost = dp[prev_mask][m] + dist[m][k];
                        best = std::min(best, cost);
                    }
                }
                
                if (best != LLONG_MAX) {
                    dp[mask][k] = best;
                }
            }
        }
    }
    
    // Final: connect back to node 0
    int full_mask = (1 << (n - 1)) - 1;  // All nodes 1..n-1
    long long best_cost = LLONG_MAX;
    
    for (int k = 1; k < n; k++) {
        if (dp[full_mask][k] != LLONG_MAX) {
            long long cost = dp[full_mask][k] + dist[k][0];
            best_cost = std::min(best_cost, cost);
        }
    }
    
    return (best_cost == LLONG_MAX) ? 0 : (int)best_cost;
}
