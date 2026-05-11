#include "held_karp.h"
#include "time_checker.h"
#include <algorithm>
#include <climits>
#include <vector>
#include <iostream>

int held_karp(const DistMatrix& dist, bool verbose, double time_limit) {
    int n = dist.size();
    if (n <= 1) return 0;
    if (n == 2) return dist[0][1] + dist[1][0];
    
    TimeChecker tc(time_limit);
    
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
        if (tc.time_exceeded()) {
            if (verbose) {
                std::cout << "Held-Karp timed out after " << tc.elapsed() << "s at subset_size=" << subset_size << std::endl;
            }
            return -1;
        }

        if (verbose) {
            std::cout << "Held-Karp processing subset_size: " << subset_size << std::endl;
        }
        
        // Iterate through all masks with exactly subset_size bits set (Gosper's hack)
        int mask = (1 << subset_size) - 1;
        while (mask < (1 << (n - 1))) {
            // For each node k in this mask
            for (int k = 1; k < n; k++) {
                if (!(mask & (1 << (k - 1)))) continue;  // if k not in mask
                
                int prev_mask = mask ^ (1 << (k - 1));
                
                // Try all possible previous nodes m in prev_mask
                long long best = LLONG_MAX;
                for (int m = 1; m < n; m++) {
                    if (m == k || !(prev_mask & (1 << (m - 1)))) continue;
                    
                    long long cost = dp[prev_mask][m] + dist[m][k];
                    best = std::min(best, cost);
                }
                
                dp[mask][k] = best;
            }
            
            // Gosper's hack: get next mask with same popcount
            int c = mask & -mask;
            int r = mask + c;
            mask = (((r ^ mask) >> 2) / c) | r;
        }
    }
    
    // Final: connect back to node 0
    int full_mask = (1 << (n - 1)) - 1;  // All nodes 1..n-1
    long long best_cost = LLONG_MAX;
    
    for (int k = 1; k < n; k++) {
        long long cost = dp[full_mask][k] + dist[k][0];
        best_cost = std::min(best_cost, cost);
    }
    
    return (int)best_cost;
}
