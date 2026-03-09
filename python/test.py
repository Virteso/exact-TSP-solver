import tsplib95

from brute_force import brute_force_tsp

problem = tsplib95.load("../tsplib/ulysses16.tsp")

brute_force_cost = brute_force_tsp(problem)

print("Brute-force cost:", brute_force_cost)
