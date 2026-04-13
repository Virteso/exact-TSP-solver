import tsplib95
import sys
from time import perf_counter as pc
import threading

# from brute_force import brute_force_tsp
from held_karp import held_karp
from branch_and_bound import branch_and_bound_tsp
from cplex_test import cplex_tsp

# problem = tsplib95.load("tsplib/gr17.tsp")

def run_with_timeout(func, args, timeout=None):
    """Run a function with an optional timeout"""
    if timeout is None:
        return func(*args), False
    
    result_container = []
    exception_container = []
    
    def target():
        try:
            result_container.append(func(*args))
        except Exception as e:
            exception_container.append(e)
    
    thread = threading.Thread(target=target, daemon=True)
    thread.start()
    thread.join(timeout)
    
    if thread.is_alive():
        return None, True  # Timed out
    
    if exception_container:
        raise exception_container[0]
    
    return result_container[0] if result_container else None, False

def build_lookup_table(problem):
    nodes = list(problem.get_nodes())
    n = len(nodes)
    lookupTable = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            weight = int(problem.get_weight(nodes[i], nodes[j]))
            lookupTable[i][j] = weight
            lookupTable[j][i] = weight
    return lookupTable


def test():
    if len(sys.argv) < 3 or len(sys.argv) > 5:
        print("Usage: python test.py <problem_file> <algorithm_choice> [timeout] [verbose]")
        print("Algorithm choice: 0 = held-karp, 1 = branch and bound, 2 = DFJ")
        print("timeout: optional timeout in seconds (default: None)")
        print("verbose: optional flag (0 or 1) for verbose output (default: 0)")
        return 0
    
    problemFile = sys.argv[1]
    algorithmChoice = int(sys.argv[2])
    timeout = float(sys.argv[3]) if len(sys.argv) > 3 else None
    if timeout == 0:
        timeout = None
    verbose = int(sys.argv[4]) if len(sys.argv) > 4 else 0

    algorithmMap = {
        0: held_karp,
        1: branch_and_bound_tsp,
        2: cplex_tsp
    }

    if algorithmChoice not in algorithmMap:
        print("Invalid algorithm choice:", algorithmChoice)
        return 0

    # Get algorithm function based on user choice
    algorithm = algorithmMap[algorithmChoice]
    algorithm_name = ["held-karp", "branch-and-bound", "cplex"][algorithmChoice]

    # Precompute the lookup table for the problem
    problem = tsplib95.load(problemFile)
    lookupTable = build_lookup_table(problem)

    if verbose:
        print(f"Running {algorithm_name} on {problemFile} (n={len(lookupTable)})")
        if timeout:
            print(f"Timeout: {timeout} seconds")

    # Warmup
    warmupRuns = 0
    for _ in range(warmupRuns):
        result, timed_out = run_with_timeout(algorithm, (lookupTable, verbose), timeout)
        if timed_out:
            print(f"Warmup run timed out after {timeout} seconds")
            return 0

    # Measured runs
    numRuns = 1
    durations = []
    for _ in range(numRuns):
        t0 = pc()
        result, timed_out = run_with_timeout(algorithm, (lookupTable, verbose), timeout)
        elapsed = pc() - t0
        
        if timed_out:
            print(f"Algorithm timed out after {timeout} seconds")
            return 0
        
        cost = result
        durations.append(elapsed)
    
    avgDuration = sum(durations) / numRuns

    name = problemFile.split("/")[-1].replace(".tsp", "")
    valid = validate_cost(name, cost)

    print(f"Cost: {cost}")
    if valid:
        print(f"Duration: {avgDuration} seconds")
        if verbose:
            print(f"Algorithm: {algorithm_name}")
            print(f"Problem: {name}")
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