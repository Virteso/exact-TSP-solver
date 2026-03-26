import itertools

def brute_force_tsp(distanceLookupTable):
    n = len(distanceLookupTable)
    nodes = list(range(n))

    start = nodes[0]
    others = nodes[1:]

    best_cost = float("inf")

    for perm in itertools.permutations(others):
        cost = distanceLookupTable[start][perm[0]] + distanceLookupTable[perm[-1]][start] + sum(distanceLookupTable[perm[i]][perm[i+1]] for i in range(n - 2))

        if cost < best_cost:
            best_cost = cost

    return best_cost