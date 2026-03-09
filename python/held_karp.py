from itertools import combinations

def held_karp(problem):
    nodes = list(problem.get_nodes())
    n = len(nodes)

    # distance lookup
    dist = {(i, j): problem.get_weight(nodes[i], nodes[j])
            for i in range(n) for j in range(n)}

    dp = {}

    # base cases
    for k in range(1, n):
        dp[(1 << k, k)] = (dist[0, k], 0)

    # build subsets
    for subset_size in range(2, n):
        for subset in combinations(range(1, n), subset_size):
            bits = sum(1 << b for b in subset)

            for k in subset:
                prev = bits & ~(1 << k)

                best = min(
                    (dp[(prev, m)][0] + dist[m, k], m)
                    for m in subset if m != k
                )

                dp[(bits, k)] = best

    bits = (1 << n) - 2

    best_cost, parent = min(
        (dp[(bits, k)][0] + dist[k, 0], k)
        for k in range(1, n)
    )

    return best_cost
