# Setup
## Quick
```
git clone https://github.com/Virteso/exact-TSP-solver.git
cd exact-TSP-solver/
bash setup.sh venv_name
```

## Step by step
Pull with submodules:  
```
git clone --recurse-submodules https://github.com/Virteso/exact-TSP-solver.git
cd exact-TSP-solver/
```

Make a python virtual environment:  
```
python -m venv <venv_name>
```

Activate the virtual environment and install dependencies:
```
source <venv_name>/bin/activate
pip install -r python/requirements.txt
```

# Algorithms

## Exact solvers

### Brute force
Checks the cost of all permutations with a fixed starting node

### Held Karp
Builds best tours in subsets and keeps a tally of the total weight, marks visited nodes using bit shifts

### Branch & bound (not implemented)
Explores the permutation tree while pruning subtrees with a lower bound higher than the current best tour.

### Branch & cut (Linear Programming) (not implemented)
Converts the TSP into a LP problam and iteratively solves that while adding cuts to invalidate subtours. Branches on fractional solutions.

## Cut constraints

### Dantzig-Fulkerson-Johnson (not implemented)

## Bound finders

### 1-tree (not implemented)

### Minimum spanning tree (not implemented)
