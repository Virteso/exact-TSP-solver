from itertools import combinations
# from Typing import List

def held_karp(distanceLookupTable: list[list[int]]) -> int:
    n = len(distanceLookupTable)

    dp = {}

    # base cases
    for k in range(1, n):
        dp[(1 << k, k)] = (distanceLookupTable[0][k], 0)

    # build subsets
    for subsetSize in range(2, n):
        for subset in combinations(range(1, n), subsetSize):
            bits = sum(1 << b for b in subset)

            for k in subset:
                prev = bits & ~(1 << k)

                best = min(
                    (dp[(prev, m)][0] + distanceLookupTable[m][k], m)
                    for m in subset if m != k
                )

                dp[(bits, k)] = best


    bits = (1 << n) - 2
    best_cost, parent = min(
        (dp[(bits, k)][0] + distanceLookupTable[k][0], k)
        for k in range(1, n)
    )

    return best_cost
