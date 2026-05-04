#ifndef CPLEX_TSP_SOLVER_H
#define CPLEX_TSP_SOLVER_H

#include <vector>

using DistMatrix = std::vector<std::vector<int>>;

// CPLEX-based ILP solver for TSP
// Uses formulation with x_ij binary variables and subtour elimination
long long cplex_tsp_solver(const DistMatrix& dist, bool verbose = false, double time_limit = 0.0);

#endif // CPLEX_TSP_SOLVER_H
