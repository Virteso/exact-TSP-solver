#ifndef LKH_SOLVER_H
#define LKH_SOLVER_H

#include <vector>

using DistMatrix = std::vector<std::vector<int>>;

long long lkh_solve(const DistMatrix& dist,
                                    int tour[],
                                    int max_trials = -1,
                                    int max_candidates = 5,
                                    int runs = 10,
                                    int move_type = 5,
                                    unsigned seed = 1,
                                    double time_limit = 0.0);

#endif
