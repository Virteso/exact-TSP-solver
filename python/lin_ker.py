from typing import List, Tuple
import random
import time


def lin_kernighan_tsp(dist: List[List[int]], max_iterations: int = 1000, 
                       random_restarts: int = 1, time_limit: float = 300.0) -> Tuple[List[int], int]:
    """
    Lin-Kernighan heuristic for TSP.
    
    Args:
        dist: 2D list where dist[i][j] is the distance between nodes i and j
        max_iterations: maximum iterations for local search
        random_restarts: number of random restart attempts
        time_limit: total time limit in seconds
    
    Returns:
        A tuple of (best_tour, cost) where best_tour is a list of node indices
    """
    n = len(dist)
    best_cost = float('inf')
    best_tour = None
    start_time = time.time()
    
    for restart in range(random_restarts):
        if time.time() - start_time > time_limit:
            break
            
        # Generate initial tour (greedy nearest neighbor or random)
        if restart == 0:
            tour = greedy_nearest_neighbor(dist)
        else:
            tour = random_tour(n)
        
        # Apply local search
        tour, cost = local_search(dist, tour, max_iterations, time_limit, start_time)
        
        if cost < best_cost:
            best_cost = cost
            best_tour = tour
    
    return best_tour, int(best_cost)


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
                 max_iterations: int, time_limit: float = 300.0, 
                 start_time: float = None) -> Tuple[List[int], int]:
    """
    Apply local search using a combination of 2-opt and 3-opt moves.
    
    Args:
        dist: distance matrix
        tour: initial tour
        max_iterations: maximum number of iterations
        time_limit: total time limit in seconds
        start_time: when the overall search started
    
    Returns:
        Improved tour and its cost
    """
    if start_time is None:
        start_time = time.time()
    
    n = len(tour)
    current_cost = calculate_tour_cost(dist, tour)
    
    # Build position index for faster lookup
    pos = [0] * n
    for idx, node in enumerate(tour):
        pos[node] = idx
    
    # Don't-look bits: skip nodes that haven't improved recently
    dont_look = [False] * n
    improving = True
    iteration = 0
    no_improve_count = 0
    
    # For large instances, limit search neighborhood
    search_radius_2opt = n - 1 if n <= 500 else max(50, n // 10)
    search_radius_3opt = min(20, n // 5) if n > 500 else min(40, n // 3)
    
    # Only apply 3-opt for smaller instances or limited passes
    use_3opt = n <= 300
    three_opt_attempts = 0
    max_3opt_attempts = 5 if n <= 200 else 2
    
    while improving and iteration < max_iterations:
        iteration += 1
        improving = False
        
        # Check time limit periodically
        if iteration % 10 == 0 and time.time() - start_time > time_limit:
            break
        
        # Try 2-opt moves with limited search
        for i in range(n):
            if dont_look[tour[i]]:
                continue
            
            # Limit j range based on problem size
            for offset in range(2, search_radius_2opt):
                j = (i + offset) % n
                if j == (i - 1) % n:
                    continue
                
                delta = two_opt_delta_fast(dist, tour, pos, i, j)
                
                if delta < -1e-9:  # Improvement found
                    # Apply 2-opt move
                    two_opt_move_inplace(tour, pos, i, j)
                    current_cost += delta
                    improving = True
                    
                    # Clear don't-look bits for affected nodes
                    dont_look[tour[i]] = False
                    dont_look[tour[j]] = False
                    dont_look[tour[(i + 1) % n]] = False
                    dont_look[tour[(j + 1) % n]] = False
                    
                    no_improve_count = 0
                    break  # Start over from next position
            
            if improving:
                break
        
        # Try 3-opt moves if 2-opt didn't improve and conditions allow
        if (not improving and use_3opt and three_opt_attempts < max_3opt_attempts 
            and time.time() - start_time < time_limit * 0.8):
            result = try_3opt_moves(dist, tour, pos, dont_look, current_cost, n, 
                                   search_radius_3opt, time_limit, start_time)
            if result is not None:
                tour, current_cost, pos, dont_look = result
                improving = True
                three_opt_attempts += 1
                no_improve_count = 0
        
        # Mark nodes as "don't-look" if no improvement after checking them
        if not improving:
            for i in range(n):
                dont_look[tour[i]] = True
            no_improve_count += 1
            
            # Reset don't-look bits occasionally for second chance
            if no_improve_count >= 3:
                dont_look = [False] * n
                no_improve_count = 0
                improving = True  # Try one more pass
    
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


def two_opt_delta_fast(dist: List[List[int]], tour: List[int], 
                       pos: List[int], i: int, j: int) -> float:
    """
    Fast 2-opt delta calculation using position array.
    Works with wraparound indices.
    
    Args:
        dist: distance matrix
        tour: current tour
        pos: position array (pos[node] = index in tour)
        i, j: positions in tour (can be wraparound)
    
    Returns:
        Cost change (negative means improvement)
    """
    n = len(tour)
    i = i % n
    j = j % n
    
    node_i = tour[i]
    node_i_next = tour[(i + 1) % n]
    node_j = tour[j]
    node_j_next = tour[(j + 1) % n]
    
    old_cost = dist[node_i][node_i_next] + dist[node_j][node_j_next]
    new_cost = dist[node_i][node_j] + dist[node_i_next][node_j_next]
    
    return new_cost - old_cost


def two_opt_move_inplace(tour: List[int], pos: List[int], i: int, j: int) -> None:
    """
    Perform a 2-opt move in-place, reversing segment between i+1 and j.
    Updates both tour and position array.
    
    Args:
        tour: tour to modify (modified in-place)
        pos: position array (modified in-place)
        i, j: positions in tour
    """
    n = len(tour)
    i = i % n
    j = j % n
    
    # Handle wraparound: ensure i < j
    if i > j:
        i, j = j, i
    
    # Reverse the segment [i+1, j]
    left = (i + 1) % n
    right = j
    
    while left < right:
        # Swap tour[left] and tour[right]
        tour[left], tour[right] = tour[right], tour[left]
        # Update position array
        pos[tour[left]] = left
        pos[tour[right]] = right
        left += 1
        right -= 1
    
    # Update position array for edges
    pos[tour[i]] = i
    pos[tour[(j + 1) % n]] = (j + 1) % n


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


def try_3opt_moves(dist: List[List[int]], tour: List[int], pos: List[int], 
                   dont_look: List[bool], current_cost: int, n: int, 
                   search_radius_3opt: int, time_limit: float, 
                   start_time: float):
    """
    Attempt to find and apply 3-opt moves.
    
    Args:
        dist: distance matrix
        tour: current tour
        pos: position array
        dont_look: don't-look bits
        current_cost: current tour cost
        n: number of nodes
        search_radius_3opt: search radius for 3-opt
        time_limit: total time limit
        start_time: when search started
    
    Returns:
        Tuple of (new_tour, new_cost, new_pos, new_dont_look) if improvement found, else None
    """
    for i in range(0, n, 2):  # Sample every 2nd position
        if time.time() - start_time > time_limit:
            break
        
        for offset_j in range(2, search_radius_3opt):
            j = (i + offset_j) % n
            if j == (i - 1) % n:
                continue
            
            for offset_k in range(offset_j + 2, offset_j + search_radius_3opt):
                k = (i + offset_k) % n
                if k == (i - 1) % n:
                    continue
                
                # Ensure i < j < k for simplicity
                ii, jj, kk = sorted([i, j, k])
                
                # Try different 3-opt reconnections
                moves = get_3opt_moves(dist, tour, ii, jj, kk)
                best_delta = 0
                best_move = None
                
                for move_tour, move_delta in moves:
                    if move_delta < best_delta:
                        best_delta = move_delta
                        best_move = move_tour
                
                if best_delta < -1e-9:
                    new_tour = best_move
                    new_cost = current_cost + best_delta
                    
                    # Rebuild position array after 3-opt
                    new_pos = [0] * n
                    for idx, node in enumerate(new_tour):
                        new_pos[node] = idx
                    
                    new_dont_look = [False] * n
                    return new_tour, new_cost, new_pos, new_dont_look
    
    return None


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
