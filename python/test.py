import tsplib95
import sys
from time import perf_counter as pc

# from brute_force import brute_force_tsp
from held_karp import held_karp

# problem = tsplib95.load("tsplib/gr17.tsp")

def build_lookup_table(problem):
    nodes = list(problem.get_nodes())
    n = len(nodes)
    lookupTable = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            weight = problem.get_weight(nodes[i], nodes[j])
            lookupTable[i][j] = weight
            lookupTable[j][i] = weight
    return lookupTable


def test():
    if len(sys.argv) != 3:
        print("Usage: python test.py <problem_file> <algorithm_choice>")
        print("Algorithm choice: 0 = held-karp")
        return 0
    
    problemFile = sys.argv[1]
    algorithmChoice = int(sys.argv[2])

    algorithmMap = {
        0: held_karp,
    }

    if algorithmChoice not in algorithmMap:
        print("Invalid algorithm choice:", algorithmChoice)
        return 0

    # Get algorithm function based on user choice
    algorithm = algorithmMap[algorithmChoice]

    # Precompute the lookup table for the problem
    problem = tsplib95.load(problemFile)
    lookupTable = build_lookup_table(problem)

    # Warmup
    warmupRuns = 5
    for _ in range(warmupRuns):
        algorithm(lookupTable)

    # Measured runs
    numRuns = 5
    durations = []
    for _ in range(numRuns):
        t0 = pc()
        cost = algorithm(lookupTable)
        durations.append(pc() - t0)
    
    avgDuration = sum(durations) / numRuns

    name = problemFile.split("/")[-1].replace(".tsp", "")
    valid = validate_cost(name, cost)

    print(f"Cost: {cost}")
    if valid:
        print(f"Duration: {avgDuration} seconds")
        return avgDuration
    else:
        print("Invalid cost for problem: ", name)
        return 0


def validate_cost(problem, testCost):
    try:
        with open("tsplib/solutions", 'r') as f:
            for line in f:
                if problem in line:
                    # Split by colon and clean up whitespace
                    costString = line.split(':')[-1]
                    cost = int(costString.strip())
                    return testCost == cost
    except FileNotFoundError:
        print("Error: Solution file not found.")
        return False
    
    return False


if __name__ == "__main__":
    test()