import heapq
from typing import Tuple, List, Dict, Set


def compute_mst_weight(dist: List[List[int]], excluded_node: int = None) -> Tuple[int, List[Tuple[int, int]]]:
    """
    Compute the Minimum Spanning Tree (MST) using Prim's algorithm,
    excluding the specified node (if any).
    
    Args:
        dist: Distance matrix (n x n) with integer edge weights
        excluded_node: Node to exclude from MST (typically node 0)
    
    Returns:
        (mst_weight, mst_edges): Total MST weight and list of edges in MST
    """
    n = len(dist)
    if n <= 1:
        return 0, []
        
    in_mst = [False] * n
    if excluded_node is not None and 0 <= excluded_node < n:
        in_mst[excluded_node] = True
    
    mst_weight = 0
    mst_edges = []
    
    # Need at least one node to start MST
    start_node = 0
    if excluded_node == 0:
        start_node = 1
    if start_node >= n:
        return 0, []
        
    in_mst[start_node] = True
    
    # Priority queue: (weight, from_node, to_node)
    pq = []
    
    # Add all edges from start_node to other nodes
    for j in range(n):
        if not in_mst[j]:
            heapq.heappush(pq, (dist[start_node][j], start_node, j))
    
    edges_added = 0
    target_edges = n - 2 if excluded_node is not None else n - 1
    while pq and edges_added < target_edges:
        weight, u, v = heapq.heappop(pq)
        
        if in_mst[v]:
            continue
        
        in_mst[v] = True
        mst_weight += weight
        mst_edges.append((u, v))
        edges_added += 1
        
        # Add edges from newly added node to other nodes
        for j in range(n):
            if not in_mst[j]:
                heapq.heappush(pq, (dist[v][j], v, j))
    
    return mst_weight, mst_edges


def compute_one_tree_bound(dist: List[List[int]], 
                           edge_weights: Dict[Tuple[int, int], int] = None) -> int:
    """
    Compute the 1-tree lower bound for TSP.
    
    A 1-tree is an MST of nodes {1, 2, ..., n-1} plus the two minimum-cost
    edges connecting node 0 to this tree.
    
    Args:
        dist: Distance matrix (n x n) with integer edge weights
        edge_weights: Optional dictionary of edge weight adjustments for Held-Karp
                     Keys are tuples (i, j) where i < j
    
    Returns:
        Lower bound value (weight of minimum 1-tree)
    """
    n = len(dist)
    
    if n < 3:
        return sum(min(dist[i][j] for j in range(n) if i != j) for i in range(n)) // 2
    
    # Apply edge weight adjustments if provided
    if edge_weights:
        adjusted_dist = [[dist[i][j] for j in range(n)] for i in range(n)]
        for (i, j), weight_adj in edge_weights.items():
            if i < j:
                adjusted_dist[i][j] -= weight_adj
                adjusted_dist[j][i] -= weight_adj
            else:
                adjusted_dist[j][i] -= weight_adj
                adjusted_dist[i][j] -= weight_adj
    else:
        adjusted_dist = dist
    
    # Compute MST of nodes {1, 2, ..., n-1}
    one_tree_weight, mst_edges = compute_mst_weight(adjusted_dist, excluded_node=0)
    
    # Find the two minimum edges from node 0 to other nodes
    first = adjusted_dist[0][1]
    second = adjusted_dist[0][2]
    if first > second:
        tmp = second
        second = first
        first = tmp

    for j in range(3, n):
        weight = adjusted_dist[0][j]
        if weight < first:
            second = first
            first = weight
        
    one_tree_weight += first
    one_tree_weight += second
    
    return one_tree_weight
