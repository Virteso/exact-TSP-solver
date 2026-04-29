#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <future>
#include <cstring>
#include <map>
#include <functional>

#include "brute_force.h"
#include "held_karp.h"
#include "branch_and_bound.h"
#include "cplex_tsp_solver.h"
#include "tsplib_parser.h"

struct AlgorithmResult {
    long long cost;
    double duration;
    bool timed_out;
};

template<typename Func>
AlgorithmResult run_with_timeout(Func func, double timeout_seconds) {
    AlgorithmResult result{-1, 0.0, false};
    auto start = std::chrono::high_resolution_clock::now();
    
    if (timeout_seconds <= 0) {
        // No timeout - run directly
        result.cost = func();
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration<double>(end - start).count();
        result.timed_out = false;
        return result;
    }
    
    // Run with timeout using future
    auto future = std::async(std::launch::async, func);
    auto timeout_duration = std::chrono::duration<double>(timeout_seconds);
    
    auto status = future.wait_for(timeout_duration);
    auto end = std::chrono::high_resolution_clock::now();
    result.duration = std::chrono::duration<double>(end - start).count();
    
    if (status == std::future_status::timeout) {
        result.timed_out = true;
        result.cost = -1;
    } else if (status == std::future_status::ready) {
        result.cost = future.get();
        result.timed_out = false;
    } else {
        result.timed_out = true;
        result.cost = -1;
    }
    
    return result;
}

AlgorithmResult run_held_karp(const DistMatrix& dist, bool verbose, double timeout_seconds) {
    return run_with_timeout([&dist, verbose]() {
        return held_karp(dist, verbose);
    }, timeout_seconds);
}

AlgorithmResult run_branch_and_bound(const DistMatrix& dist, bool verbose, double timeout_seconds) {
    return run_with_timeout([&dist, verbose]() {
        return branch_and_bound_tsp(dist, verbose);
    }, timeout_seconds);
}

AlgorithmResult run_brute_force(const DistMatrix& dist, bool verbose, double timeout_seconds) {
    return run_with_timeout([&dist, verbose]() {
        return brute_force_tsp(dist, verbose);
    }, timeout_seconds);
}

AlgorithmResult run_cplex(const DistMatrix& dist, bool verbose, double timeout_seconds) {
    return run_with_timeout([&dist, verbose]() {
        return cplex_tsp_solver(dist, verbose);
    }, timeout_seconds);
}

bool validate_cost(const std::string& problem_name, long long cost) {
    std::ifstream solutions_file("tsplib/solutions");
    if (!solutions_file.is_open()) {
        std::cerr << "Error: Solution file not found at tsplib/solutions\n";
        return false;
    }
    
    std::string line;
    while (std::getline(solutions_file, line)) {
        if (line.find(problem_name) != std::string::npos) {
            size_t colon_pos = line.rfind(':');
            if (colon_pos != std::string::npos) {
                std::string cost_str = line.substr(colon_pos + 1);
                // Trim whitespace
                cost_str.erase(0, cost_str.find_first_not_of(" \t"));
                cost_str.erase(cost_str.find_last_not_of(" \t") + 1);
                
                try {
                    long long expected_cost = std::stoll(cost_str);
                    solutions_file.close();
                    return cost == expected_cost;
                } catch (...) {
                    solutions_file.close();
                    return false;
                }
            }
        }
    }
    
    solutions_file.close();
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <problem_file> <algorithm_choice> [verbose] [timeout]\n";
        std::cerr << "Algorithm choice: 0 = brute force, 1 = held-karp, 2 = branch and bound, 3 = cplex\n";
        std::cerr << "verbose: optional flag (0 or 1) for verbose output (default: 0)\n";
        std::cerr << "timeout: optional timeout in seconds (default: None)\n";
        return 1;
    }
    
    std::string problem_file = argv[1];
    int algorithm_choice = std::stoi(argv[2]);
    bool verbose = (argc > 3) ? (std::stoi(argv[3]) != 0) : false;
    double timeout = (argc > 4) ? std::stod(argv[4]) : 0.0;
    
    // Algorithm map with names
    std::map<int, std::string> algorithm_names = {
        {0, "brute-force"},
        {1, "held-karp"},
        {2, "branch-and-bound"},
        {3, "cplex"}
    };
    
    if (algorithm_names.find(algorithm_choice) == algorithm_names.end()) {
        std::cerr << "Invalid algorithm choice: " << algorithm_choice << std::endl;
        return 1;
    }
    
    try {
        // Parse problem file
        TSPProblem problem = parse_tsp_file(problem_file);
        DistMatrix lookup_table = build_lookup_table(problem);
        
        if (verbose) {
            std::cout << "Running " << algorithm_names[algorithm_choice] 
                      << " on " << problem_file 
                      << " (n=" << lookup_table.size() << ")\n";
            if (timeout > 0) {
                std::cout << "Timeout: " << timeout << " seconds\n";
            }
        }
        
        std::map<int, std::function<AlgorithmResult(const DistMatrix&, bool, double)>> algorithm_map = {
            {0, run_brute_force},
            {1, run_held_karp},
            {2, run_branch_and_bound},
            {3, run_cplex}
        };
        
        AlgorithmResult result = algorithm_map[algorithm_choice](lookup_table, verbose, timeout);
        
        if (result.timed_out) {
            std::cout << "Algorithm timed out after " << timeout << " seconds\n";
            return 124;
        }
        
        if (result.cost < 0) {
            std::cout << "Algorithm failed to compute result\n";
            return 1;
        }

        std::cout << "Cost: " << result.cost << std::endl;
        
        // Extract problem name
        size_t last_slash = problem_file.rfind('/');
        std::string name = problem_file.substr(last_slash + 1);
        name = name.substr(0, name.find(".tsp"));
        
        if (validate_cost(name, result.cost)) {
            std::cout << "Duration: " << result.duration << " seconds\n";
            if (verbose) {
                std::cout << "Algorithm: " << algorithm_names[algorithm_choice] << std::endl;
                std::cout << "Problem: " << name << std::endl;
            }
            return 0;
        } else {
            std::cout << "Invalid cost for problem: " << name << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
