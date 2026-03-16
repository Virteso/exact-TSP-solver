import tsplib95
import sys
from time import perf_counter as pc

# from brute_force import brute_force_tsp
from held_karp import held_karp

# problem = tsplib95.load("tsplib/gr17.tsp")

def test():
    if len(sys.argv) != 3:
        print("Usage: python test.py <problem_file> <algorithm_choice>")
        print("Algorithm choice: 0 = held-karp")
        return 0
    
    problem_file = sys.argv[1]
    algorithm_choice = int(sys.argv[2])

    problem = tsplib95.load(problem_file)

    algorithm_map = {
        0: held_karp,
    }

    if algorithm_choice not in algorithm_map:
        print("Invalid algorithm choice:", algorithm_choice)
        return 0


    algorithm = algorithm_map[algorithm_choice]
    # Warmup
    warmup_runs = 5
    for _ in range(warmup_runs):
        algorithm(problem)

    # Measured runs
    num_runs = 5
    durations = []
    for _ in range(num_runs):
        t0 = pc()
        cost = algorithm(problem)
        durations.append(pc() - t0)
    
    avg_duration = sum(durations) / num_runs

    name = problem_file.split("/")[-1].replace(".tsp", "")
    valid = validate_cost(name, cost)

    print(f"Cost: {cost}")
    if valid:
        print(f"Duration: {avg_duration} seconds")
        return avg_duration
    else:
        print("Invalid cost for problem: ", name)
        return 0


def validate_cost(problem, test_cost):
    try:
        with open("tsplib/solutions", 'r') as f:
            for line in f:
                if problem in line:
                    # Split by colon and clean up whitespace
                    cost_string = line.split(':')[-1]
                    cost = int(cost_string.strip())
                    return test_cost == cost
    except FileNotFoundError:
        print("Error: Solution file not found.")
        return False
    
    return False


if __name__ == "__main__":
    test()