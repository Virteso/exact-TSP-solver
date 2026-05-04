#include <iostream>
#include <vector>
#include <cmath>

#include "time_checker.h"
#include "lkh_solver.h"

#define MIP_INT_LB 0.8

using DistMatrix = std::vector<std::vector<int>>;

#ifdef CPLEX_AVAILABLE

#include <ilconcert/ilosys.h>
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN


// Alias for a 2D array of CPLEX decision variables
typedef IloArray<IloNumVarArray> IloNumVarArray2;

class SubtourCallback : public IloCplex::Callback::Function {
private:
    IloNumVarArray2 x;
    int n;

public:
    // Constructor passes the variable matrix and problem size
    SubtourCallback(const IloNumVarArray2& vars, int size) : x(vars), n(size) {}

    // The invoke method is called by CPLEX during the branch-and-bound process
    virtual void invoke(const IloCplex::Callback::Context& context) override {
        
        // Help CPLEX only with relaxation and integer solutions
        if (!context.inCandidate() && !context.inRelaxation()) return;

        // Extract the current values of the variables from the candidate context
        int adj[n];
        for (int i = 0; i < n; ++i) {
            adj[i] = -1;
            for (int j = 0; j < n; ++j) {
                if (i != j && context.getCandidatePoint(x[i][j]) > MIP_INT_LB) {
                    adj[i] = j;
                }
            }
        }
        
        // Find cycles in the current integer solution
        bool visited[n];
        std::vector<std::vector<int>> tours;

        for (int i = 0; i < n; i++) {
            if (!visited[i] && adj[i] != -1) {
                std::vector<int> tour;
                int curr = i;
                while (!visited[curr]) {
                    visited[curr] = true;
                    tour.push_back(curr);
                    curr = adj[curr];
                    if (curr == -1) break; // Should not happen in valid TSP
                }
                tours.push_back(tour);
            }
        }

        // A single tour solution is the correct one
        if (tours.size() == 1)
            return;

        for (auto &tour : tours) {
            int cycle_length = static_cast<int>(tour.size());
            IloEnv env = context.getEnv();
            IloExpr expr(env);
            for (int a = 0; a < cycle_length; ++a) {
                for (int b = 0; b < cycle_length; ++b) {
                    if (tour[a] != tour[b]) {
                        expr += x[tour[a]][tour[b]];
                    }
                }
            }
            
            // Reject the candidate and inject the cut
            context.rejectCandidate(expr <= cycle_length - 1);
            
            expr.end();
        }
    }
    
    virtual ~SubtourCallback() {}
};


long long cplex_tsp_solver(const DistMatrix& dist, bool verbose, double time_limit) {
    int n = dist.size();
    
    // Trivial cases
    if (n <= 1) return 0;
    if (n == 2) return dist[0][1] + dist[1][0];

    TimeChecker tc(time_limit);
    
    double lk_time_limit;
    if (time_limit > 0.0)
        lk_time_limit = std::min(time_limit, std::max(10.0, n / 20.0));
    else
        lk_time_limit = std::max(10, n / 20);

    int lk_tour[n];
    long long optimal_cost = lkh_solve(dist, lk_tour, n, 5, 10, 5, 1, lk_time_limit);

    if (verbose)
        std::cout << "LKH upper bound: " << optimal_cost << std::endl;

    IloEnv env;

    try {
        IloModel model(env);

        // x[i][j] = 1 if edge from i to j is in the tour, 0 otherwise
        IloNumVarArray2 x(env, n);
        for (int i = 0; i < n; ++i) {
            x[i] = IloNumVarArray(env, n, 0, 1, ILOBOOL);
        }

        // Minimize sum(dist[i][j] * x[i][j])
        IloExpr objExpr(env);
        for (int i = 0; i < n; ++i) {
            // Prevent self-loops explicitly
            model.add(x[i][i] == 0);
            // Add edge costs
            for (int j = 0; j < i; ++j) {
                    objExpr += dist[i][j] * x[i][j];    
                    objExpr += dist[j][i] * x[j][i];    
            }
        }
        model.add(IloMinimize(env, objExpr));
        objExpr.end();

        // Degree Constraints (Only 1 edge in and only 1 edge out)
        for (int i = 0; i < n; ++i) {
            IloExpr outDegree(env);
            IloExpr inDegree(env);
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    outDegree += x[i][j];
                    inDegree += x[j][i];
                }
            }
            model.add(outDegree == 1);
            model.add(inDegree == 1);
            outDegree.end();
            inDegree.end();
        }

        // Initialize CPLEX
        IloCplex cplex(model);

        if (!verbose) {
            cplex.setOut(env.getNullStream());
            cplex.setWarning(env.getNullStream());
        }

        cplex.setParam(IloCplex::Param::Threads, 0);
        // cplex.setParam(IloCplex::Param::Parallel, IloCplex::Opportunistic);
        cplex.setParam(IloCplex::Param::Parallel, IloCplex::Deterministic);

        // IMPORTANT: When using lazy constraints, we must prevent CPLEX 
        // from doing dual reductions in presolve, otherwise it might cut off 
        // the optimal solution assuming the relaxed model is the whole story.
        cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 1); // 1 = Primal reductions only

        // Absolutely optimal results
        cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 0.0);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::AbsMIPGap, 0.0);

        // Register the Context Callback
        SubtourCallback subtourCb(x, n);
        cplex.use(&subtourCb, IloCplex::Callback::Context::Id::Candidate);

        // Give CPLEX whatever time is left
        if (time_limit > 0.0) {
            cplex.setParam(IloCplex::Param::TimeLimit, tc.remaining());
        }
        
        // Provide MIP start with Lin-Kernighan solution as initial incumbent
        if (optimal_cost > 0) {
            IloNumVarArray startVar(env);
            IloNumArray startVal(env);
            
            // Convert the tour to x[i][j] variables
            // Tour is a permutation of nodes, so x[lk_tour[i]][lk_tour[i+1]] = 1
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    startVar.add(x[i][j]);
                    // Check if edge (i,j) is in the tour
                    bool in_tour = false;
                    for (int t = 0; t < n; ++t) {
                        if (lk_tour[t] == i && lk_tour[(t + 1) % n] == j) {
                            in_tour = true;
                        }
                    }
                    startVal.add(in_tour ? 1 : 0);
                }
            }
            
            cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartEffort::MIPStartAuto, "LK_solution");
            startVar.end();
            startVal.end();

            // Disable cplex heuristics with a good upper bound
            cplex.setParam(IloCplex::Param::MIP::Strategy::RINSHeur, -1);
            // cplex.setParam(IloCplex::Param::Emphasis::MIP, 3);
        }

        // Solve the Model
        if (cplex.solve()) {
            if (verbose) {
                std::cout << "Solution status: " << cplex.getStatus() << std::endl;
            }
            optimal_cost = std::round(cplex.getObjValue());
        } else {
            if (verbose) {
                std::cout << "No solution found." << std::endl;
            }
        }
        
    } catch (const IloException& e) {
        std::cerr << "CPLEX Exception: " << e << std::endl;
    } catch (...) {
        std::cerr << "Unknown C++ exception occurred." << std::endl;
    }

    env.end();
    return optimal_cost;
}

#else

long long cplex_tsp_solver(const DistMatrix& dist, bool verbose, double time_limit) {
    std::cerr << "CPLEX not available. Please link against CPLEX libraries." << std::endl;
    return -1;
}

#endif // CPLEX_AVAILABLE