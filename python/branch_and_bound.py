import heapq
from typing import List
from one_tree import compute_one_tree_bound, compute_mst_weight, held_karp_lower_bound
from lin_ker import lin_kernighan_tsp

def branch_and_bound_tsp(lookupTable: List[List[int]]) -> int: 
	"""
	Branch and bound algorithm for TSP.
	Uses lower bound estimation to prune the search tree.
	
	Args:
		lookupTable: 2D list where lookupTable[i][j] is the distance between nodes i and j
	"""
	n = len(lookupTable)

	if n <= 1:
		return 0

	best_cost = lin_kernighan_tsp(lookupTable)
	
	# Priority queue: (lower_bound, current_cost, path_indices, visited_mask)
	# Start from node 0
	# lower_bound, current_cost, current_node, visited_mask
	pq = [
		(
			0,	# lower_bound
			0,	# current_cost
			0,	# current_node
			1	# visited_mask
		)
	]
	
	while pq:
		lower_bound, current_cost, current_node, visited_mask = heapq.heappop(pq)
		
		# Pruning: if lower bound exceeds best found, skip this branch
		if lower_bound >= best_cost:
			continue
		print(f"lower bound: {lower_bound}, best: {best_cost}")

		unvisited = [
			next_node
			for next_node in range(n) 
			if not (visited_mask & (1 << next_node))
		]
		
		if len(unvisited) == 1:
			node = unvisited[0]
			# Try path: current_node -> node -> 0
			cost = current_cost + lookupTable[current_node][node] + lookupTable[node][0]
			best_cost = min(best_cost, cost)
			continue

		
		# # Compute MST on all unvisited nodes once
		mst_weight, _ = compute_mst_weight(build_submatrix(lookupTable, unvisited), excluded_node=None)
		mst_bound_unvisited = mst_weight
		
		for next_node in unvisited:
			
			new_cost = current_cost + lookupTable[current_node][next_node]
			
			# Calculate lower bound for remaining nodes
			remaining_unvisited_list = [node for node in unvisited if node != next_node]
			
			# Fast approximation: use the pre-computed MST on ALL unvisited nodes.
			# A valid lower bound for travelling through remaining_unvisited and ending at 0 is:
			# MST(unvisited) - (longest edge connected to next_node in MST) + min_edge_to_0
			# To keep it strictly simple and fast, MST(all unvisited) is a valid lower bound 
			# for the path spanning remaining_unvisited + the edge from next_node into them.
			# Because next_node is part of unvisited, MST(unvisited) spans all of them.
			min_edge_to_0 = min(lookupTable[node][0] for node in remaining_unvisited_list)
			new_lower_bound = new_cost + mst_bound_unvisited + min_edge_to_0
			
			# Only add to queue if lower bound is promising
			if new_lower_bound < best_cost:
				heapq.heappush(
					pq,
					(new_lower_bound, new_cost, next_node, visited_mask | (1 << next_node))
				)
	
	return best_cost


def build_submatrix(dist: List[List[int]], nodes: List[int]) -> List[List[int]]:
	"""
	Build a submatrix for a list of nodes.
	
	Args:
		dist: full distance matrix
		nodes: list of node indices
		
	Returns:
		Submatrix containing only the specified nodes
	"""
	sub_n = len(nodes)
	sub_dist = [[0] * sub_n for _ in range(sub_n)]
	
	for i in range(sub_n):
		for j in range(i, sub_n):
			sub_dist[i][j] = dist[nodes[i]][nodes[j]]
			sub_dist[j][i] = dist[nodes[j]][nodes[i]]
	
	return sub_dist
