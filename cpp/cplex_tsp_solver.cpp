#include <iostream>
#include <vector>
#include <cmath>
#include "lin_kernighan.h"

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
        
        // We only want to execute this logic when CPLEX finds a new integer candidate
        if (!context.inCandidate()) return;

        // Extract the current values of the variables from the candidate context
        std::vector<int> adj(n, -1);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j && context.getCandidatePoint(x[i][j]) > 0.5) {
                    adj[i] = j;
                }
            }
        }
        
        // Find cycles in the current integer solution
        std::vector<bool> visited(n, false);
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

        if (tours.size() == 1)
            return;

        for (auto &tour : tours) {
        // If the cycle length is less than n, we found an illegal subtour.
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
            // // Reject only 1 subtour
            // break;
        }
    }
    
    virtual ~SubtourCallback() {}
};


long long cplex_tsp_solver(const DistMatrix& dist, bool verbose = false) {
    int n = dist.size();
    
    // Trivial cases
    if (n <= 1) return 0;
    if (n == 2) return dist[0][1] + dist[1][0];

    IloEnv env;
    long long optimal_cost = -1;

    try {
        IloModel model(env);

        // x[i][j] = 1 if edge from i to j is in the tour, 0 otherwise
        IloNumVarArray2 x(env, n);
        for (int i = 0; i < n; ++i) {
            x[i] = IloNumVarArray(env, n, 0, 1, ILOBOOL);
        }

        // 1. Objective Function: Minimize sum(dist[i][j] * x[i][j])
        IloExpr objExpr(env);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    objExpr += dist[i][j] * x[i][j];
                } else {
                    // Prevent self-loops explicitly
                    model.add(x[i][j] == 0); 
                }
            }
        }
        model.add(IloMinimize(env, objExpr));
        objExpr.end();

        // 2. Degree Constraints (Assignment constraints)
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

        cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 0.0);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::AbsMIPGap, 0.0);

        // 3. Register the Context Callback
        // We instruct CPLEX to trigger this callback ONLY in the 'Candidate' context
        SubtourCallback subtourCb(x, n);
        cplex.use(&subtourCb, IloCplex::Callback::Context::Id::Candidate);

        // 4. Get upper bound and tour from Lin-Kernighan heuristic
        auto [lk_tour, lk_bound] = lin_kernighan_tsp_with_tour(dist, 1000, 5);
        if (verbose) {
            std::cout << "Lin-Kernighan upper bound: " << lk_bound << std::endl;
        }
        
        // Provide MIP start with Lin-Kernighan solution as initial incumbent
        if (lk_bound > 0 && !lk_tour.empty()) {
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
                            break;
                        }
                    }
                    startVal.add(in_tour ? 1 : 0);
                }
            }
            
            cplex.addMIPStart(startVar, startVal, IloCplex::MIPStartEffort::MIPStartAuto, "LK_solution");
            startVar.end();
            startVal.end();
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

long long cplex_tsp_solver(const DistMatrix& dist, bool verbose) {
    std::cerr << "CPLEX not available. Please link against CPLEX libraries." << std::endl;
    return -1;
}

#endif // CPLEX_AVAILABLE