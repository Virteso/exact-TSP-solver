import itertools

def brute_force_tsp(lookupTable):
    n = len(lookupTable)
    nodes = list(range(n))

    start = nodes[0]
    others = nodes[1:]

    best_cost = float("inf")

    for perm in itertools.permutations(others):
        cost = lookupTable[start][perm[0]] + lookupTable[perm[-1]][start] + sum(lookupTable[perm[i]][perm[i+1]] for i in range(n - 2))

        if cost < best_cost:
            best_cost = cost

    return best_cost