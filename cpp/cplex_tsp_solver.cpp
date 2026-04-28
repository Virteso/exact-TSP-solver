#include <iostream>
#include <vector>
#include <cmath>

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
        std::vector<std::vector<double>> sol(n, std::vector<double>(n, 0.0));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    sol[i][j] = context.getCandidatePoint(x[i][j]);
                }
            }
        }
        
        // Find cycles in the current integer solution
        std::vector<bool> visited(n, false);
        for (int i = 0; i < n; ++i) {
            if (visited[i]) continue;

            std::vector<int> tour;
            int curr = i;
            
            // Traverse the path for the current subtour
            while (!visited[curr]) {
                visited[curr] = true;
                tour.push_back(curr);
                
                int next = -1;
                for (int j = 0; j < n; ++j) {
                    if (curr != j && sol[curr][j] > 0.5) {
                        next = j;
                        break;
                    }
                }
                
                if (next == -1) break; // Failsafe
                curr = next;
            }
            
            // If the cycle length is less than n, we found an illegal subtour.
            if (tour.size() > 0 && tour.size() < static_cast<size_t>(n)) {
                IloEnv env = context.getEnv();
                IloExpr expr(env);
                for (size_t a = 0; a < tour.size(); ++a) {
                    for (size_t b = 0; b < tour.size(); ++b) {
                        if (tour[a] != tour[b]) {
                            expr += x[tour[a]][tour[b]];
                        }
                    }
                }
                
                // Reject the candidate and inject the cut
                context.rejectCandidate(expr <= static_cast<int>(tour.size()) - 1);
                
                expr.end();
                // Reject only 1 subtour
                break;
            }
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