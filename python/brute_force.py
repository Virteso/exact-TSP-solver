import itertools

def brute_force_tsp(problem):
    nodes = list(problem.get_nodes())

    start = nodes[0]
    others = nodes[1:]

    best_cost = float("inf")

    for perm in itertools.permutations(others):
        tour = (start,) + perm + (start,)

        cost = sum(problem.get_weight(tour[i], tour[i+1])
                   for i in range(len(tour)-1))

        if cost < best_cost:
            best_cost = cost

    return best_cost