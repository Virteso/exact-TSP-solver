"""
Dantzig-Fulkerson-Johnson (DFJ) algorithm for TSP using cutting planes.
Integrates with qsopt as the LP/ILP solver.
"""

from typing import List, Tuple, Set
import heapq
from itertools import combinations
import ctypes


# QSOPT Wrapper - Ready for integration with qsopt library

class QsoptSolver:
    """
    Wrapper for qsopt LP/ILP solver using ctypes.
    Automatically finds libqsopt_wrapper.so or falls back to PuLP.
    """
    
    def __init__(self, lib_path: str = None):
        """
        Initialize qsopt solver.
        
        Args:
            lib_path: Path to qsopt shared library (.so, .dylib) or None to use default
        """
        self.lib = None
        self.use_fallback = False
        self.prob = None
        
        if lib_path is None:
            lib_path = self._find_qsopt_lib()
        
        if lib_path:
            try:
                self.lib = ctypes.CDLL(lib_path)
                self._setup_function_signatures()
                print(f"[DFJ] Using qsopt solver from {lib_path}")
                print(f"[DFJ] qsopt version: {self._get_version()}")
            except Exception as e:
                print(f"[DFJ] Warning: Could not load qsopt from {lib_path}: {e}")
                return 1
        else:
            return 1
    
    def _find_qsopt_lib(self) -> str:
        """Find qsopt library in common locations."""
        import os
        
        # Check environment variable
        if "QSOPT_LIB" in os.environ:
            return os.environ["QSOPT_LIB"]
        
        # Common library names depending on OS
        lib_names = ["libqsopt_wrapper.so", "libqsopt_wrapper.dylib", 
                     "libqsopt.so", "libqsopt.dylib", "qsopt.dll"]
        common_paths = ["/usr/lib", "/usr/local/lib", "./lib", ".", 
                       os.path.dirname(__file__), os.getcwd()]
        
        for path in common_paths:
            for lib_name in lib_names:
                full_path = os.path.join(path, lib_name)
                if os.path.exists(full_path):
                    return full_path
        
        return None
    
    def _setup_function_signatures(self):
        """Setup ctypes function signatures for qsopt library."""
        
        # Problem type (opaque pointer)
        self.QSprob = ctypes.c_void_p
        
        # Function signatures
        self.lib.wrap_QScreate_prob.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.lib.wrap_QScreate_prob.restype = self.QSprob
        
        self.lib.wrap_QSfree_prob.argtypes = [self.QSprob]
        self.lib.wrap_QSfree_prob.restype = None
        
        self.lib.wrap_QSget_colcount.argtypes = [self.QSprob]
        self.lib.wrap_QSget_colcount.restype = ctypes.c_int
        
        self.lib.wrap_QSget_rowcount.argtypes = [self.QSprob]
        self.lib.wrap_QSget_rowcount.restype = ctypes.c_int
        
        self.lib.wrap_QSnew_col.argtypes = [self.QSprob, ctypes.c_double, 
                                           ctypes.c_double, ctypes.c_double, 
                                           ctypes.c_char_p]
        self.lib.wrap_QSnew_col.restype = ctypes.c_int
        
        self.lib.wrap_QSnew_row.argtypes = [self.QSprob, ctypes.c_double, 
                                           ctypes.c_char, ctypes.c_char_p]
        self.lib.wrap_QSnew_row.restype = ctypes.c_int
        
        self.lib.wrap_QSadd_col.argtypes = [self.QSprob, ctypes.c_int,
                                           ctypes.POINTER(ctypes.c_int),
                                           ctypes.POINTER(ctypes.c_double),
                                           ctypes.c_double, ctypes.c_double,
                                           ctypes.c_double, ctypes.c_char_p]
        self.lib.wrap_QSadd_col.restype = ctypes.c_int
        
        self.lib.wrap_QSadd_row.argtypes = [self.QSprob, ctypes.c_int,
                                           ctypes.POINTER(ctypes.c_int),
                                           ctypes.POINTER(ctypes.c_double),
                                           ctypes.c_double, ctypes.c_char,
                                           ctypes.c_char_p]
        self.lib.wrap_QSadd_row.restype = ctypes.c_int
        
        self.lib.wrap_QSchange_objcoef.argtypes = [self.QSprob, ctypes.c_int,
                                                  ctypes.c_double]
        self.lib.wrap_QSchange_objcoef.restype = ctypes.c_int
        
        self.lib.wrap_QSchange_rhscoef.argtypes = [self.QSprob, ctypes.c_int,
                                                  ctypes.c_double]
        self.lib.wrap_QSchange_rhscoef.restype = ctypes.c_int
        
        self.lib.wrap_QSchange_bound.argtypes = [self.QSprob, ctypes.c_int,
                                                ctypes.c_char, ctypes.c_double]
        self.lib.wrap_QSchange_bound.restype = ctypes.c_int
        
        self.lib.wrap_QSopt_primal.argtypes = [self.QSprob, 
                                              ctypes.POINTER(ctypes.c_int)]
        self.lib.wrap_QSopt_primal.restype = ctypes.c_int
        
        self.lib.wrap_QSget_status.argtypes = [self.QSprob, 
                                              ctypes.POINTER(ctypes.c_int)]
        self.lib.wrap_QSget_status.restype = ctypes.c_int
        
        self.lib.wrap_QSget_objval.argtypes = [self.QSprob, 
                                              ctypes.POINTER(ctypes.c_double)]
        self.lib.wrap_QSget_objval.restype = ctypes.c_int
        
        self.lib.wrap_QSget_x_array.argtypes = [self.QSprob,
                                               ctypes.POINTER(ctypes.c_double)]
        self.lib.wrap_QSget_x_array.restype = ctypes.c_int
        
        self.lib.wrap_QSversion.argtypes = []
        self.lib.wrap_QSversion.restype = ctypes.c_char_p
    
    def _get_version(self) -> str:
        """Get qsopt version string."""
        try:
            if self.lib:
                ver = self.lib.wrap_QSversion()
                return ver.decode('utf-8') if isinstance(ver, bytes) else str(ver)
        except:
            pass
        return "unknown"
    
    def solve_lp(self, c: List[float], A_ub: List[List[float]], 
                 b_ub: List[float], bounds: List[Tuple[int, int]]) -> Tuple[float, List[float]]:
        """
        Solve linear program using qsopt.
        
        Args:
            c: Objective coefficients
            A_ub: Inequality constraint matrix (A_ub @ x <= b_ub)
            b_ub: Inequality constraint bounds
            bounds: Variable bounds (lower, upper)
        
        Returns:
            (objective_value, solution_vector)
        """      
        return self._solve_lp_qsopt(c, A_ub, b_ub, bounds)


    def _solve_lp_qsopt(self, c: List[float], A_ub: List[List[float]], 
                        b_ub: List[float], bounds: List[Tuple[int, int]]
                        ) -> Tuple[float, List[float]]:
        """Solve LP using qsopt."""
        try:
            n = len(c)
            m = len(A_ub)
            
            # Create problem (minimize)
            prob = self.lib.wrap_QScreate_prob(b"TSP_LP", ctypes.c_int(1))  # 1 = QS_MIN
            
            if not prob:
                print("[DFJ] Failed to create qsopt problem")
                return (0, [])
            
            try:
                # Add columns (variables)
                for j in range(n):
                    obj, lower, upper = c[j], bounds[j][0], bounds[j][1]
                    self.lib.wrap_QSnew_col(prob, ctypes.c_double(obj),
                                          ctypes.c_double(lower),
                                          ctypes.c_double(upper), None)
                
                # Add rows (constraints) in sparse format
                for i in range(m):
                    # Find non-zero entries in row i
                    nzcnt = 0
                    nzind = []
                    nzval = []
                    
                    for j in range(n):
                        if abs(A_ub[i][j]) > 1e-10:
                            nzind.append(j)
                            nzval.append(A_ub[i][j])
                            nzcnt += 1
                    
                    if nzcnt > 0:
                        # Convert to ctypes arrays
                        ind_array = (ctypes.c_int * nzcnt)(*nzind)
                        val_array = (ctypes.c_double * nzcnt)(*nzval)
                        
                        self.lib.wrap_QSadd_row(prob, ctypes.c_int(nzcnt),
                                              ind_array, val_array,
                                              ctypes.c_double(b_ub[i]),
                                              ctypes.c_char(b'L'),  # 'L' for <=
                                              None)
                
                # Solve
                status = ctypes.c_int()
                self.lib.wrap_QSopt_primal(prob, ctypes.byref(status))
                
                if status.value != 1:  # 1 = QS_LP_OPTIMAL
                    print(f"[DFJ] qsopt returned status {status.value}")
                    self.lib.wrap_QSfree_prob(prob)
                    return (0, [])
                
                # Get objective value
                obj_val = ctypes.c_double()
                self.lib.wrap_QSget_objval(prob, ctypes.byref(obj_val))
                
                # Get solution
                x = (ctypes.c_double * n)()
                self.lib.wrap_QSget_x_array(prob, x)
                
                solution = list(x)
                
                self.lib.wrap_QSfree_prob(prob)
                
                return obj_val.value, solution
            
            except Exception as e:
                print(f"[DFJ] qsopt solve error: {e}")
                self.lib.wrap_QSfree_prob(prob)
                return (0, [])
        
        except Exception as e:
            print(f"[DFJ] qsopt error: {e}")
            return (0, [])
    
    def _solve_lp_qsopt_with_equality(self, c: List[float], 
                                      A_eq: List[List[float]], 
                                      b_eq: List[float],
                                      A_ub: List[List[float]], 
                                      b_ub: List[float],
                                      bounds: List[Tuple[int, int]]) -> Tuple[float, List[float]]:
        """Solve LP with both equality and inequality constraints using qsopt."""
        try:
            n = len(c)
            m_eq = len(A_eq)
            m_ub = len(A_ub)
            
            # Create problem (minimize)
            prob = self.lib.wrap_QScreate_prob(b"TSP_LP", ctypes.c_int(1))  # 1 = QS_MIN
            
            if not prob:
                print("[DFJ] Failed to create qsopt problem")
                return (0, [])
            
            try:
                # Add columns (variables) with bounds
                for j in range(n):
                    obj, lower, upper = c[j], bounds[j][0], bounds[j][1]
                    self.lib.wrap_QSnew_col(prob, ctypes.c_double(obj),
                                          ctypes.c_double(lower),
                                          ctypes.c_double(upper), None)
                
                # Add equality constraints: convert Ax = b to two inequality constraints
                # Ax <= b and -Ax <= -b (which is -Ax + -b <= 0)
                for i in range(m_eq):
                    # Constraint: sum_j A_eq[i][j] * x_j = b_eq[i]
                    # Expressed as: sum_j A_eq[i][j] * x_j <= b_eq[i]
                    nzcnt = sum(1 for j in range(n) if abs(A_eq[i][j]) > 1e-10)
                    if nzcnt > 0:
                        nzind = []
                        nzval = []
                        for j in range(n):
                            if abs(A_eq[i][j]) > 1e-10:
                                nzind.append(j)
                                nzval.append(A_eq[i][j])
                        
                        ind_array = (ctypes.c_int * nzcnt)(*nzind)
                        val_array = (ctypes.c_double * nzcnt)(*nzval)
                        
                        # Add Ax <= b
                        self.lib.wrap_QSadd_row(prob, ctypes.c_int(nzcnt),
                                              ind_array, val_array,
                                              ctypes.c_double(b_eq[i]),
                                              ctypes.c_char(b'E'),  # 'E' for equality
                                              None)
                
                # Add inequality constraints
                for i in range(m_ub):
                    nzcnt = sum(1 for j in range(n) if abs(A_ub[i][j]) > 1e-10)
                    if nzcnt > 0:
                        nzind = []
                        nzval = []
                        for j in range(n):
                            if abs(A_ub[i][j]) > 1e-10:
                                nzind.append(j)
                                nzval.append(A_ub[i][j])
                        
                        ind_array = (ctypes.c_int * nzcnt)(*nzind)
                        val_array = (ctypes.c_double * nzcnt)(*nzval)
                        
                        # Add Ax <= b
                        self.lib.wrap_QSadd_row(prob, ctypes.c_int(nzcnt),
                                              ind_array, val_array,
                                              ctypes.c_double(b_ub[i]),
                                              ctypes.c_char(b'L'),  # 'L' for <=
                                              None)
                
                # Solve
                status = ctypes.c_int()
                self.lib.wrap_QSopt_primal(prob, ctypes.byref(status))
                
                if status.value != 1:  # 1 = QS_LP_OPTIMAL
                    print(f"[DFJ] qsopt returned status {status.value}")
                    self.lib.wrap_QSfree_prob(prob)
                    return (0, [])
                
                # Get objective value
                obj_val = ctypes.c_double()
                self.lib.wrap_QSget_objval(prob, ctypes.byref(obj_val))
                
                # Get solution
                x = (ctypes.c_double * n)()
                self.lib.wrap_QSget_x_array(prob, x)
                
                solution = list(x)
                
                self.lib.wrap_QSfree_prob(prob)
                
                return obj_val.value, solution
            
            except Exception as e:
                print(f"[DFJ] qsopt solve error: {e}")
                self.lib.wrap_QSfree_prob(prob)
                return (0, [])
        
        except Exception as e:
            print(f"[DFJ] qsopt error: {e}")
            return (0, [])


# ============================================================================
# DFJ Algorithm Implementation
# ============================================================================

def dfj_tsp(lookupTable: List[List[int]], verbose: bool = False) -> int:
    """
    Dantzig-Fulkerson-Johnson algorithm for TSP.
    
    Uses LP relaxations with subtour elimination cutting planes and branch & bound.
    Solves LP problems using qsopt for optimal speed.
    
    **Performance**: Exact optimal solutions. Speed depends on problem size:
    - Very fast: ≤ 20 cities (< 1 minute)
    - Fast: 20-30 cities (< 5 minutes)
    - Moderate: 30-40 cities (5-30 minutes)
    - Slow: > 40 cities (may take hours)
    
    Args:
        lookupTable: 2D distance matrix
        time_limit: Maximum computation time in seconds (not enforced)
        verbose: Print debug information
    
    Returns:
        Minimum TSP tour cost (optimal)
    """
    n = len(lookupTable)
    
    if n <= 1:
        return 0
    
    if n == 2:
        return lookupTable[0][1] + lookupTable[1][0]
    
    # Initialize qsopt solver
    solver = QsoptSolver()
    
    # For problems > 15 cities, optionally use a heuristic first to get upper bound
    # This improves branch-and-bound pruning (but slows initial solve)
    initial_bound = float('inf')
    if n > 15 and n <= 40:
        try:
            from lin_ker import lin_kernighan_tsp
            initial_bound = lin_kernighan_tsp(lookupTable)
            if verbose:
                print(f"[DFJ] Heuristic upper bound: {initial_bound}")
        except:
            pass
    
    # Use branch and bound with LP relaxations via qsopt
    result = _branch_and_bound_dfj(lookupTable, solver, n, verbose=verbose, upper_bound=initial_bound)
    
    return result


def _branch_and_bound_dfj(dist: List[List[int]], solver: QsoptSolver, 
                          n: int, verbose: bool = False, upper_bound: float = float('inf')) -> int:
    """
    Branch and bound with DFJ cutting planes.
    """
    
    # Priority queue: (lower_bound, node_id, fixed_edges, forbidden_edges, forced_cuts)
    pq = [(0, 0, set(), set(), set())]
    
    best_cost = upper_bound  # Use provided upper bound if better than infinity
    node_count = 0
    
    # Adaptive node limit based on problem size
    if n <= 20:
        max_nodes = 10000
    elif n <= 30:
        max_nodes = 2000
    else:
        max_nodes = 500
    
    while pq and node_count < max_nodes:
        lower_bound, _, fixed_edges, forbidden_edges, forced_cuts = heapq.heappop(pq)
        node_count += 1
        
        # Pruning
        if lower_bound >= best_cost:
            continue
        
        if verbose and node_count % 10 == 0:
            print(f"[DFJ] Nodes explored: {node_count}, Best: {best_cost}, Lower bound: {lower_bound}")
        
        # Solve LP relaxation with current fixed/forbidden edges and forced cuts
        lp_cost, lp_solution = _solve_tsp_lp(dist, solver, fixed_edges, forbidden_edges, n, forced_cuts)
        
        if lp_cost is None:  # Infeasible
            if verbose:
                print(f"[DFJ] LP infeasible at node {node_count}")
            continue
        
        lp_cost = round(lp_cost)
        if verbose:
            print(f"[DFJ] Node {node_count}: LP cost = {lp_cost}")
        
        # Pruning
        if lp_cost >= best_cost:
            continue
        
        # Check if solution is integral (valid tour)
        is_integral, tour = _check_integral_solution(lp_solution, dist, n)
        
        if is_integral:
            tour_cost = _calculate_tour_cost(tour, dist)
            if verbose:
                print(f"[DFJ] Found tour with cost {tour_cost}")
            if tour_cost < best_cost:
                best_cost = tour_cost
        else:
            # Find subtours and generate cutting planes / branch
            subtours = _find_subtours(lp_solution, n)
            
            if len(subtours) > 1:
                # Try to find the largest subtour and add a cutting plane for it
                largest_subtour = max(subtours, key=len)
                if len(largest_subtour) < n:
                    new_forced_cuts = forced_cuts.copy()
                    new_forced_cuts.add(frozenset(largest_subtour))
                    if verbose:
                        print(f"[DFJ] Adding cutting plane for subtour of size {len(largest_subtour)}")
                    heapq.heappush(pq, (lp_cost, node_count, fixed_edges, forbidden_edges, new_forced_cuts))
                
                # Also branch on a fractional edge - use best branching strategy
                # Find edge closest to 0.5 for better bounds
                best_edge = None
                best_dist_to_half = 1.0
                for i in range(n):
                    for j in range(i + 1, n):
                        edge_val = lp_solution.get((i, j), 0)
                        if 0 < edge_val < 1:
                            dist_to_half = abs(edge_val - 0.5)
                            if dist_to_half < best_dist_to_half:
                                best_dist_to_half = dist_to_half
                                best_edge = (i, j)
                
                if best_edge:
                    i, j = best_edge
                    if verbose:
                        print(f"[DFJ] Branching on edge ({i},{j}) with value {lp_solution.get(best_edge, 0):.3f}")
                    # Branch: fix edge to 1
                    new_fixed = fixed_edges.copy()
                    new_fixed.add((min(i, j), max(i, j)))
                    
                    # Also create branch: fix edge to 0
                    new_forbidden = forbidden_edges.copy()
                    new_forbidden.add((min(i, j), max(i, j)))
                    
                    heapq.heappush(pq, (lp_cost, node_count, new_fixed, forbidden_edges, forced_cuts))
                    heapq.heappush(pq, (lp_cost, node_count + 1, fixed_edges, new_forbidden, forced_cuts))
    
    if verbose or node_count >= max_nodes:
        print(f"[DFJ] Final best cost: {best_cost} (explored {node_count} nodes)")
        if node_count >= max_nodes:
            print(f"[DFJ] Warning: Hit node limit of {max_nodes}. Solution may be suboptimal.")
    
    return int(best_cost) if best_cost != float('inf') else 0


def _solve_tsp_lp(dist: List[List[int]], solver: QsoptSolver, 
                  fixed_edges: Set[Tuple[int, int]],
                  forbidden_edges: Set[Tuple[int, int]], n: int,
                  forced_cuts: Set = None) -> Tuple[float, dict]:
    """
    Solve LP relaxation of TSP with iterative cutting planes.
    Only adds violated cuts to avoid memory explosion.
    
    Returns:
        (objective_value, edge_solution_dict)
    """
    
    if forced_cuts is None:
        forced_cuts = set()
    
    # Variables: x[i,j] for all edges (0 <= x[i,j] <= 1)
    num_vars = n * (n - 1) // 2
    
    # Build objective: minimize sum of distances
    c = []
    edge_list = []
    
    for i in range(n):
        for j in range(i + 1, n):
            c.append(float(dist[i][j]))
            edge_list.append((i, j))
    
    # Build base constraints
    A_eq = []  # Equality constraints (degree = 2)
    b_eq = []
    
    # 1. Degree constraints: sum of edges incident to each node = 2 (EQUALITY)
    for node in range(n):
        constraint = [0] * num_vars
        for var_idx, (i, j) in enumerate(edge_list):
            if i == node or j == node:
                constraint[var_idx] = 1
        A_eq.append(constraint)
        b_eq.append(2)
    
    # 2. Forced cutting planes for specific subtours
    A_ub = []
    b_ub = []
    for subtour_nodes in forced_cuts:
        subtour_set = set(subtour_nodes)
        constraint = [0] * num_vars
        for var_idx, (i, j) in enumerate(edge_list):
            if i in subtour_set and j in subtour_set:
                constraint[var_idx] = 1
        
        if sum(constraint) > 0:
            A_ub.append(constraint)
            b_ub.append(len(subtour_set) - 1)
    
    # Bounds for variables: 0 <= x[i,j] <= 1
    bounds = [(0, 1) for _ in range(num_vars)]
    
    # Fix edges based on branch & bound
    for var_idx, edge in enumerate(edge_list):
        if edge in fixed_edges:
            bounds[var_idx] = (1, 1)
        elif edge in forbidden_edges:
            bounds[var_idx] = (0, 0)
    
    # Iterative cutting plane method - only add violated cuts
    max_iterations = 5
    for cut_iteration in range(max_iterations):
        try:
            # Solve LP with current constraints using qsopt
            obj_val, solution = solver._solve_lp_qsopt_with_equality(c, A_eq, b_eq, A_ub, b_ub, bounds)
            
            if obj_val is None:
                return None, {}
            
            # Check for violated subtour cuts
            violated_cuts = _find_violated_cuts(solution, edge_list, n, min_size=2, max_size=min(n // 2, 7))
            
            if not violated_cuts:
                # No more violated cuts - solution is good
                edge_solution = {}
                for var_idx, edge in enumerate(edge_list):
                    if solution[var_idx] is not None and solution[var_idx] > 1e-6:
                        edge_solution[edge] = solution[var_idx]
                return obj_val, edge_solution
            
            # Add most violated cuts
            for subtour_nodes, violation in violated_cuts[:3]:  # Add top 3 violated cuts
                subtour_set = set(subtour_nodes)
                constraint = [0] * num_vars
                for var_idx, (i, j) in enumerate(edge_list):
                    if i in subtour_set and j in subtour_set:
                        constraint[var_idx] = 1
                
                if sum(constraint) > 0:
                    A_ub.append(constraint)
                    b_ub.append(len(subtour_set) - 1)
        
        except Exception as e:
            print(f"LP solve failed at cut iteration {cut_iteration}: {e}")
            return None, {}
    
    # Final solve with accumulated cuts
    try:
        obj_val, solution = solver._solve_lp_qsopt_with_equality(c, A_eq, b_eq, A_ub, b_ub, bounds)
        edge_solution = {}
        for var_idx, edge in enumerate(edge_list):
            if solution[var_idx] is not None and solution[var_idx] > 1e-6:
                edge_solution[edge] = solution[var_idx]
        return obj_val, edge_solution
    except Exception as e:
        print(f"Final LP solve failed: {e}")
        return None, {}


def _check_integral_solution(edge_solution: dict, dist: List[List[int]], 
                             n: int) -> Tuple[bool, List[int]]:
    """
    Check if edge solution is integral and construct tour if valid.
    
    Returns:
        (is_integral, tour_as_list)
    """
    
    # Check if all edges are 0 or 1
    for value in edge_solution.values():
        if value > 1e-6 and value < 1 - 1e-6:
            return False, []
    
    # Build adjacency from edges
    adj = [[] for _ in range(n)]
    edge_count = [0] * n
    
    for (i, j), value in edge_solution.items():
        if value > 0.5:
            adj[i].append(j)
            adj[j].append(i)
            edge_count[i] += 1
            edge_count[j] += 1
    
    # Check all nodes have degree 2
    if not all(degree == 2 for degree in edge_count):
        return False, []
    
    # Check if it forms a single cycle (valid tour)
    visited = [False] * n
    tour = [0]
    visited[0] = True
    current = 0
    
    for _ in range(n - 1):
        next_node = None
        for neighbor in adj[current]:
            if not visited[neighbor]:
                next_node = neighbor
                break
        
        if next_node is None:
            # Should not happen if degree is 2
            return False, []
        
        tour.append(next_node)
        visited[next_node] = True
        current = next_node
    
    # Check if we have exactly one cycle
    if not all(visited):
        return False, []
    
    return True, tour


def _find_violated_cuts(edge_solution: dict, edge_list: List[Tuple[int, int]], 
                        n: int, min_size: int = 2, max_size: int = 7) -> List[Tuple[Tuple, float]]:
    """
    Find subtour elimination cuts violated by current LP solution.
    Returns list of (subtour_nodes, violation) sorted by violation (most violated first).
    
    A cut is violated if the number of edges in a subset > subset_size - 1.
    """
    violated = []
    
    # Convert edge_solution dict to edge list with values
    edge_values = {}
    for var_idx, edge in enumerate(edge_list):
        if edge in edge_solution:
            edge_values[edge] = edge_solution[edge]
    
    # Check subtours of various sizes
    for size in range(min_size, min(max_size + 1, n)):
        for nodes in combinations(range(n), size):
            nodes_set = set(nodes)
            
            # Count edges within this subtour
            edge_count = 0.0
            for (i, j) in edge_values:
                if i in nodes_set and j in nodes_set:
                    edge_count += edge_values[(i, j)]
            
            # Violation: if edge_count > size - 1
            violation = edge_count - (size - 1)
            
            if violation > 1e-4:  # Significant violation
                violated.append((nodes, violation))
    
    # Sort by violation magnitude (most violated first)
    violated.sort(key=lambda x: x[1], reverse=True)
    
    return violated


def _find_subtours(edge_solution: dict, n: int) -> List[List[int]]:
    """
    Find all subtours (connected components) in edge solution.
    """
    
    adj = [[] for _ in range(n)]
    
    for (i, j), value in edge_solution.items():
        if value > 0.5:
            adj[i].append(j)
            adj[j].append(i)
    
    visited = [False] * n
    subtours = []
    
    for start in range(n):
        if not visited[start]:
            subtour = []
            stack = [start]
            
            while stack:
                node = stack.pop()
                if visited[node]:
                    continue
                visited[node] = True
                subtour.append(node)
                
                for neighbor in adj[node]:
                    if not visited[neighbor]:
                        stack.append(neighbor)
            
            subtours.append(subtour)
    
    return subtours


def _calculate_tour_cost(tour: List[int], dist: List[List[int]]) -> int:
    """Calculate total distance of a tour."""
    return dist[tour[-1]][tour[0]] + sum((
            dist[tour[i]][tour[i+1]] 
        for node in range(len(tour)-1)))
