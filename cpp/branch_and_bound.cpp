#include "branch_and_bound.h"
#include "one_tree.h"
#include "lin_kernighan.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <iostream>

struct BranchNode {
    long long lower_bound;
    long long current_cost;
    int current_node;
    int visited_mask;
    
    bool operator>(const BranchNode& other) const {
        return lower_bound > other.lower_bound;
    }
};

std::vector<std::vector<int>> build_submatrix(const DistMatrix& dist, 
                                               const std::vector<int>& nodes) {
    int n = nodes.size();
    std::vector<std::vector<int>> sub(n, std::vector<int>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            sub[i][j] = dist[nodes[i]][nodes[j]];
        }
    }
    return sub;
}

long long branch_and_bound_tsp(const DistMatrix& dist_matrix, bool verbose) {
    int n = dist_matrix.size();
    
    if (n <= 1) return 0;
    if (n == 2) return dist_matrix[0][1] + dist_matrix[1][0];
    
    // Get initial upper bound from heuristic
    long long best_cost = lin_kernighan_tsp(dist_matrix, 1000, 1);
    
    // Priority queue: min heap by lower bound
    std::priority_queue<BranchNode, std::vector<BranchNode>, std::greater<BranchNode>> pq;
    
    pq.push({0, 0, 0, 1});  // Start from node 0
    
    int nodes_explored = 0;
    
    while (!pq.empty()) {
        BranchNode node = pq.top();
        pq.pop();
        
        nodes_explored++;
        
        // Pruning
        if (node.lower_bound >= best_cost) {
            continue;
        }
        
        if (verbose && nodes_explored % 1000 == 0) {
            std::cout << "lower bound: " << node.lower_bound 
                      << ", best: " << best_cost << std::endl;
        }
        
        // Get unvisited nodes
        std::vector<int> unvisited;
        for (int i = 0; i < n; i++) {
            if (!(node.visited_mask & (1 << i))) {
                unvisited.push_back(i);
            }
        }
        
        // If only one unvisited node left
        if (unvisited.size() == 1) {
            int last_node = unvisited[0];
            long long cost = node.current_cost + dist_matrix[node.current_node][last_node] 
                           + dist_matrix[last_node][0];
            best_cost = std::min(best_cost, cost);
            continue;
        }
        
        // Compute MST on unvisited nodes for lower bound
        DistMatrix sub = build_submatrix(dist_matrix, unvisited);
        auto [mst_weight, _] = compute_mst_weight(sub);
        
        // Try each unvisited node as next
        for (int next_node : unvisited) {
            long long new_cost = node.current_cost + dist_matrix[node.current_node][next_node];
            
            // Remove next_node from unvisited and recompute lower bound
            std::vector<int> remaining;
            for (int u : unvisited) {
                if (u != next_node) remaining.push_back(u);
            }
            
            long long min_edge_to_0 = std::numeric_limits<long long>::max();
            for (int u : remaining) {
                min_edge_to_0 = std::min(min_edge_to_0, (long long)dist_matrix[u][0]);
            }
            
            long long new_lower_bound = new_cost + mst_weight + min_edge_to_0;
            
            if (new_lower_bound < best_cost) {
                pq.push({
                    new_lower_bound,
                    new_cost,
                    next_node,
                    node.visited_mask | (1 << next_node)
                });
            }
        }
    }
    
    return best_cost;
}
