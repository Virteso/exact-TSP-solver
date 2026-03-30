from typing import List, Tuple
import random


def lin_kernighan_tsp(dist: List[List[int]], max_iterations: int = 1000, 
                       random_restarts: int = 1) -> int:
    """
    Lin-Kernighan heuristic for TSP.
    
    Args:
        dist: 2D list where dist[i][j] is the distance between nodes i and j
        max_iterations: maximum iterations for local search
        random_restarts: number of random restart attempts
    
    Returns:
        The cost of the best tour found
    """
    n = len(dist)
    best_cost = float('inf')
    best_tour = None
    
    for restart in range(random_restarts):
        # Generate initial tour (greedy nearest neighbor or random)
        if restart == 0:
            tour = greedy_nearest_neighbor(dist)
        else:
            tour = random_tour(n)
        
        # Apply local search
        tour, cost = local_search(dist, tour, max_iterations)
        
        if cost < best_cost:
            best_cost = cost
            best_tour = tour
    
    return best_cost


def greedy_nearest_neighbor(dist: List[List[int]], start: int = 0) -> List[int]:
    """
    Greedy nearest neighbor algorithm to build initial tour.
    
    Args:
        dist: distance matrix
        start: starting node
    
    Returns:
        A tour as a list of node indices
    """
    n = len(dist)
    unvisited = set(range(n))
    tour = [start]
    unvisited.remove(start)
    
    current = start
    while unvisited:
        nearest = min(unvisited, key=lambda x: dist[current][x])
        tour.append(nearest)
        unvisited.remove(nearest)
        current = nearest
    
    return tour


def random_tour(n: int) -> List[int]:
    """
    Generate a random tour.
    
    Args:
        n: number of nodes
    
    Returns:
        A random tour as a list of node indices
    """
    tour = list(range(n))
    random.shuffle(tour)
    return tour


def calculate_tour_cost(dist: List[List[int]], tour: List[int]) -> int:
    """
    Calculate the total cost of a tour.
    
    Args:
        dist: distance matrix
        tour: tour as a list of node indices
    
    Returns:
        Total cost of the tour
    """
    cost = 0
    n = len(tour)
    for i in range(n):
        cost += dist[tour[i]][tour[(i + 1) % n]]
    return cost


def local_search(dist: List[List[int]], tour: List[int], 
                 max_iterations: int) -> Tuple[List[int], int]:
    """
    Apply local search using a combination of 2-opt and 3-opt moves.
    
    Args:
        dist: distance matrix
        tour: initial tour
        max_iterations: maximum number of iterations
    
    Returns:
        Improved tour and its cost
    """
    n = len(dist)
    current_cost = calculate_tour_cost(dist, tour)
    
    improving = True
    iteration = 0
    
    while improving and iteration < max_iterations:
        iteration += 1
        improving = False
        
        # Try 2-opt moves
        for i in range(n):
            for j in range(i + 2, n):
                if j == n - 1 and i == 0:
                    continue
                
                delta = two_opt_delta(dist, tour, i, j)
                
                if delta < -1e-9:  # Improvement found
                    tour = two_opt_move(tour, i, j)
                    current_cost += delta
                    improving = True
                    break

            if improving:
                break
        
        # Try 3-opt moves if no 2-opt improvement found
        if not improving and iteration < max_iterations // 2:
            for i in range(n):
                for j in range(i + 2, min(i + n - 1, n)):
                    for k in range(j + 2, min(i + n, n)):
                        if k == n - 1 and i == 0:
                            continue
                        
                        # Try different 3-opt reconnections
                        moves = get_3opt_moves(dist, tour, i, j, k)
                        best_delta = 0
                        best_move = None
                        
                        for move_tour, move_delta in moves:
                            if move_delta < best_delta:
                                best_delta = move_delta
                                best_move = move_tour
                        
                        if best_delta < -1e-9:
                            tour = best_move
                            current_cost += best_delta
                            improving = True
                            break
                    
                    if improving:
                        break
                
                if improving:
                    break
    
    return tour, int(current_cost)


def two_opt_delta(dist: List[List[int]], tour: List[int], i: int, k: int) -> float:
    """
    Calculate the cost change for a 2-opt move.
    Reverses the segment between positions i+1 and k.
    
    Args:
        dist: distance matrix
        tour: current tour
        i: first position
        k: second position (k > i+1)
    
    Returns:
        Cost change (negative means improvement)
    """
    n = len(tour)
    
    old_cost = (dist[tour[i]][tour[i + 1]] + 
                dist[tour[k]][tour[(k + 1) % n]])
    
    new_cost = (dist[tour[i]][tour[k]] + 
                dist[tour[i + 1]][tour[(k + 1) % n]])
    
    return new_cost - old_cost


def two_opt_move(tour: List[int], i: int, k: int) -> List[int]:
    """
    Perform a 2-opt move by reversing the segment between positions i+1 and k.
    
    Args:
        tour: current tour
        i: first position
        k: second position
    
    Returns:
        New tour after the move
    """
    new_tour = tour[:i + 1] + tour[i + 1:k + 1][::-1] + tour[k + 1:]
    return new_tour


def get_3opt_moves(dist: List[List[int]], tour: List[int], i: int, j: int, k: int) -> List[Tuple[List[int], float]]:
    """
    Generate possible 3-opt reconnections and their cost changes.
    
    Args:
        dist: distance matrix
        tour: current tour
        i, j, k: positions in the tour where i < j < k
    
    Returns:
        List of (new_tour, cost_delta) tuples
    """
    n = len(tour)
    moves = []
    
    # Current edges involved in 3-opt
    current_cost = (dist[tour[i]][tour[i + 1]] +
                   dist[tour[j]][tour[j + 1]] +
                   dist[tour[k]][tour[(k + 1) % n]])
    
    # 3-opt move 1
    new_tour_1 = tour[:i + 1] + tour[i + 1:j + 1][::-1] + tour[j + 1:]
    new_cost_1 = (dist[tour[i]][tour[j]] +
                  dist[tour[i + 1]][tour[j + 1]] +
                  dist[tour[k]][tour[(k + 1) % n]])
    delta_1 = new_cost_1 - current_cost
    moves.append((new_tour_1, delta_1))
    
    # 3-opt move 2
    new_tour_2 = tour[:j + 1] + tour[j + 1:k + 1][::-1] + tour[k + 1:]
    new_cost_2 = (dist[tour[i]][tour[i + 1]] +
                  dist[tour[j]][tour[k]] +
                  dist[tour[j + 1]][tour[(k + 1) % n]])
    delta_2 = new_cost_2 - current_cost
    moves.append((new_tour_2, delta_2))
    
    # 3-opt move 3
    new_tour_3 = tour[:i + 1] + tour[i + 1:j + 1][::-1] + tour[j + 1:k + 1][::-1] + tour[k + 1:]
    new_cost_3 = (dist[tour[i]][tour[j]] +
                  dist[tour[i + 1]][tour[k]] +
                  dist[tour[j + 1]][tour[(k + 1) % n]])
    delta_3 = new_cost_3 - current_cost
    moves.append((new_tour_3, delta_3))
    
    return moves
