from itertools import combinations

def held_karp(lookupTable):
    n = len(lookupTable)

    dp = {}

    # base cases
    for k in range(1, n):
        dp[(1 << k, k)] = (lookupTable[0][k], 0)

    # build subsets
    for subsetSize in range(2, n):
        for subset in combinations(range(1, n), subsetSize):
            bits = sum(1 << b for b in subset)

            for k in subset:
                prev = bits & ~(1 << k)

                best = min(
                    (dp[(prev, m)][0] + lookupTable[m][k], m)
                    for m in subset if m != k
                )

                dp[(bits, k)] = best


    bits = (1 << n) - 2
    best_cost, parent = min(
        (dp[(bits, k)][0] + lookupTable[k][0], k)
        for k in range(1, n)
    )

    return best_cost
