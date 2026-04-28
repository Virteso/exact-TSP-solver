#include "one_tree.h"
#include <queue>
#include <limits>
#include <algorithm>

std::pair<long long, std::vector<std::pair<int, int>>> 
compute_mst_weight(const DistMatrix& dist, int excluded_node) {
    int n = dist.size();
    if (n <= 1) return {0, {}};
    
    std::vector<bool> in_mst(n, false);
    if (excluded_node >= 0 && excluded_node < n) {
        in_mst[excluded_node] = true;
    }
    
    long long mst_weight = 0;
    std::vector<std::pair<int, int>> mst_edges;
    
    // Start from first non-excluded node
    int start_node = (excluded_node == 0) ? 1 : 0;
    if (start_node >= n) return {0, {}};
    
    in_mst[start_node] = true;
    
    // Priority queue: (weight, from_node, to_node)
    using Edge = std::tuple<int, int, int>;
    auto cmp = [](const Edge& a, const Edge& b) { 
        return std::get<0>(a) > std::get<0>(b); 
    };
    std::priority_queue<Edge, std::vector<Edge>, decltype(cmp)> pq(cmp);
    
    // Add edges from start_node
    for (int j = 0; j < n; j++) {
        if (!in_mst[j]) {
            pq.push({dist[start_node][j], start_node, j});
        }
    }
    
    int target_edges = (excluded_node >= 0) ? (n - 2) : (n - 1);
    int edges_added = 0;
    
    while (!pq.empty() && edges_added < target_edges) {
        auto [weight, u, v] = pq.top();
        pq.pop();
        
        if (in_mst[v]) continue;
        
        in_mst[v] = true;
        mst_weight += weight;
        mst_edges.push_back({u, v});
        edges_added++;
        
        // Add edges from newly added node
        for (int j = 0; j < n; j++) {
            if (!in_mst[j]) {
                pq.push({dist[v][j], v, j});
            }
        }
    }
    
    return {mst_weight, mst_edges};
}

long long compute_one_tree_bound(const DistMatrix& dist) {
    int n = dist.size();
    
    if (n < 3) {
        long long sum = 0;
        for (int i = 0; i < n; i++) {
            int min_edge = std::numeric_limits<int>::max();
            for (int j = 0; j < n; j++) {
                if (i != j) min_edge = std::min(min_edge, dist[i][j]);
            }
            sum += min_edge;
        }
        return sum / 2;
    }
    
    // MST of nodes {1, 2, ..., n-1}
    auto [mst_weight, mst_edges] = compute_mst_weight(dist, 0);
    
    // Two minimum edges from node 0 to other nodes
    int first = dist[0][1];
    int second = dist[0][2];
    if (first > second) std::swap(first, second);
    
    for (int j = 3; j < n; j++) {
        int weight = dist[0][j];
        if (weight < first) {
            second = first;
            first = weight;
        } else if (weight < second) {
            second = weight;
        }
    }
    
    return mst_weight + first + second;
}
