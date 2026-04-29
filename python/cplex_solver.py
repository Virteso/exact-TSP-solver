import cplex
from cplex.callbacks import Context
from lin_ker import lin_kernighan_tsp

class SubtourCallback:
    def __init__(self, n):
        self.n = n

    def invoke(self, context):
        # Only act if CPLEX has found a candidate integer solution
        if not context.in_candidate():
            return

        # Get current solution (the x_ij values)
        n = self.n
        sol = context.get_candidate_point()

        # Find subtours
        visited = [False] * n
        all_subtours = []
        
        for start_node in range(n):
            if visited[start_node]:
                continue

            # New subtour
            tour = []
            curr = start_node
            while not visited[curr]:
                visited[curr] = True
                tour.append(curr)
                # Find next node in path: where x[curr][j] == 1
                next = -1
                for j in range(n):
                    if curr == j: continue
                    # Variables were added in order: i*n + j
                    if sol[curr * n + j] > 0.5:
                        next = j
                        break
                if next == -1:
                    continue
                curr = next

            all_subtours.append(tour)

        if len(all_subtours) == 1:
            return

        # Eliminate subtours
        for tour in all_subtours:
            # Create cut: sum(x_ij for i,j in subtour) <= |S| - 1
            indices = [i * n + j 
                    for i in tour 
                    for j in tour 
                    if i != j]
            
            # Reject solution and provide cut
            context.reject_candidate(
                constraints=[cplex.SparsePair(ind=indices, val=[1.0] * len(indices))],
                senses="L",
                rhs=[float(len(tour) - 1)]
            )


def cplex_tsp(dist_matrix, verbose = None) -> float:
    n = len(dist_matrix)
    prob = cplex.Cplex()
    
    if verbose is False:
        prob.set_results_stream(None)
        prob.set_warning_stream(None)
        prob.set_error_stream(None)
        prob.set_log_stream(None)

    # Standard Setup
    prob.objective.set_sense(prob.objective.sense.minimize)
    
    # Variables: x_ij
    obj = [dist_matrix[i][j] for i in range(n) for j in range(n)]
    prob.variables.add(obj=obj, types="B" * (n * n))
    
    # Degree Constraints
    for i in range(n):
        # Out
        prob.linear_constraints.add(
            lin_expr=[cplex.SparsePair(ind=[i*n + j for j in range(n) if i!=j], val=[1.0]*(n-1))],
            senses="E", rhs=[1.0]
        )
        # In
        prob.linear_constraints.add(
            lin_expr=[cplex.SparsePair(ind=[j*n + i for j in range(n) if i!=j], val=[1.0]*(n-1))],
            senses="E", rhs=[1.0]
        )

    # MANDATORY: To use Generic Callbacks with Lazy Constraints,
    # we must disable some preprocessing that changes the model structure.
    prob.parameters.preprocessing.linear.set(1)

    # Set exact optimality tolerances
    prob.parameters.mip.tolerances.mipgap.set(0.0)
    prob.parameters.mip.tolerances.absmipgap.set(0.0)

    # Get upper bound and tour from Lin-Kernighan heuristic
    lk_tour, lk_bound = lin_kernighan_tsp(dist_matrix, max_iterations=1000, random_restarts=2)
    if verbose:
        print(f"Lin-Kernighan upper bound: {lk_bound}")
    
    # Add MIP start with Lin-Kernighan solution
    if lk_bound > 0 and lk_tour is not None:
        # Convert tour to x_ij variables: x[i][j] = 1 if edge (i,j) is in tour
        mip_start = []
        for i in range(n):
            for j in range(n):
                # Check if edge (i,j) is in the tour
                in_tour = False
                for t in range(n):
                    if lk_tour[t] == i and lk_tour[(t + 1) % n] == j:
                        in_tour = True
                        break
                mip_start.append(1.0 if in_tour else 0.0)
        
        # Add as MIP start
        prob.MIP_starts.add(mip_start, prob.MIP_starts.effort_level.auto)

    # 4. Register the Generic Callback
    # We tell CPLEX to only call us when it finds a potential integer solution
    prob.set_callback(SubtourCallback(n), Context.id.candidate)

    prob.solve()
    return round(prob.solution.get_objective_value())
