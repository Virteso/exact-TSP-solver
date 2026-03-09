import tsplib95

from brute_force import brute_force_tsp
from held_karp import held_karp

problem = tsplib95.load("tsplib/gr17.tsp")

held_karp_cost = held_karp(problem)

print("Held-Karp cost:", held_karp_cost)
