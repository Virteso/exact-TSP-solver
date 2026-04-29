#include "lin_kernighan.h"
#include <algorithm>
#include <random>
#include <limits>
#include <vector>

static std::mt19937 g_rng(std::random_device{}());

long long calculate_tour_cost(const DistMatrix& dist, const std::vector<int>& tour) {
    long long cost = 0;
    int n = tour.size();
    for (int i = 0; i < n; i++) {
        cost += dist[tour[i]][tour[(i + 1) % n]];
    }
    return cost;
}

std::vector<int> greedy_nearest_neighbor(const DistMatrix& dist, int start = 0) {
    int n = dist.size();
    std::vector<bool> visited(n, false);
    std::vector<int> tour;
    tour.push_back(start);
    visited[start] = true;
    
    int current = start;
    for (int i = 1; i < n; i++) {
        int nearest = -1;
        int min_dist = std::numeric_limits<int>::max();
        for (int j = 0; j < n; j++) {
            if (!visited[j] && dist[current][j] < min_dist) {
                min_dist = dist[current][j];
                nearest = j;
            }
        }
        tour.push_back(nearest);
        visited[nearest] = true;
        current = nearest;
    }
    
    return tour;
}

std::vector<int> random_tour(int n) {
    std::vector<int> tour(n);
    for (int i = 0; i < n; i++) tour[i] = i;
    std::shuffle(tour.begin(), tour.end(), g_rng);
    return tour;
}

std::vector<int> two_opt_move(const std::vector<int>& tour, int i, int k) {
    std::vector<int> new_tour;
    new_tour.insert(new_tour.end(), tour.begin(), tour.begin() + i + 1);
    new_tour.insert(new_tour.end(), tour.rbegin() + (tour.size() - k - 1), tour.rbegin() + (tour.size() - i - 1));
    new_tour.insert(new_tour.end(), tour.begin() + k + 1, tour.end());
    return new_tour;
}

void two_opt_move_inplace(std::vector<int>& tour, std::vector<int>& pos, int i, int j) {
    int n = tour.size();
    i = (i + n) % n;
    j = (j + n) % n;
    
    // Handle wraparound: ensure i < j
    if (i > j) std::swap(i, j);
    
    // Reverse segment [i+1, j]
    int left = (i + 1) % n;
    int right = j;
    
    while (left < right) {
        std::swap(tour[left], tour[right]);
        pos[tour[left]] = left;
        pos[tour[right]] = right;
        left++;
        right--;
    }
    
    // Update position array for boundary nodes
    pos[tour[i]] = i;
    pos[tour[(j + 1) % n]] = (j + 1) % n;
}

long long two_opt_delta(const DistMatrix& dist, const std::vector<int>& tour, int i, int k) {
    int n = tour.size();
    long long old_cost = dist[tour[i]][tour[i + 1]] + dist[tour[k]][tour[(k + 1) % n]];
    long long new_cost = dist[tour[i]][tour[k]] + dist[tour[i + 1]][tour[(k + 1) % n]];
    return new_cost - old_cost;
}

// Helper function to try 3-opt moves
bool try_3opt_moves(const DistMatrix& dist, std::vector<int>& tour, std::vector<int>& pos, 
                    std::vector<bool>& dont_look, long long& current_cost, int n) {
    int search_radius_3opt = std::min(20, n / 5);
    
    for (int i = 0; i < n; i += 2) {  // Sample every 2nd position
        for (int offset_j = 2; offset_j < search_radius_3opt; offset_j++) {
            int j = (i + offset_j) % n;
            if (j == (i + n - 1) % n) continue;
            
            for (int offset_k = offset_j + 2; offset_k < std::min(offset_j + search_radius_3opt, n - 1); offset_k++) {
                int k = (i + offset_k) % n;
                if (k == (i + n - 1) % n) continue;
                
                // Try 3-opt on segment [i, j, k]
                // Check move 1: reverse [i+1, j]
                long long cost_0 = dist[tour[i]][tour[i+1]] + dist[tour[j]][tour[j+1]] + dist[tour[k]][tour[(k+1)%n]];
                long long cost_1 = dist[tour[i]][tour[j]] + dist[tour[i+1]][tour[j+1]] + dist[tour[k]][tour[(k+1)%n]];
                
                if (cost_1 - cost_0 < -1e-9) {
                    // Apply move 1
                    std::vector<int> new_tour = tour;
                    std::reverse(new_tour.begin() + i + 1, new_tour.begin() + j + 1);
                    long long new_cost = calculate_tour_cost(dist, new_tour);
                    if (new_cost < current_cost) {
                        tour = new_tour;
                        current_cost = new_cost;
                        // Rebuild position array
                        for (int idx = 0; idx < n; idx++) {
                            pos[tour[idx]] = idx;
                        }
                        dont_look.assign(n, false);
                        return true;
                    }
                }
                
                // Check move 2: reverse [j+1, k]
                long long cost_2 = dist[tour[i]][tour[i+1]] + dist[tour[j]][tour[k]] + dist[tour[j+1]][tour[(k+1)%n]];
                if (cost_2 - cost_0 < -1e-9) {
                    // Apply move 2
                    std::vector<int> new_tour = tour;
                    std::reverse(new_tour.begin() + j + 1, new_tour.begin() + k + 1);
                    long long new_cost = calculate_tour_cost(dist, new_tour);
                    if (new_cost < current_cost) {
                        tour = new_tour;
                        current_cost = new_cost;
                        // Rebuild position array
                        for (int idx = 0; idx < n; idx++) {
                            pos[tour[idx]] = idx;
                        }
                        dont_look.assign(n, false);
                        return true;
                    }
                }
                
                // Check move 3: reverse both [i+1, j] and [j+1, k]
                long long cost_3 = dist[tour[i]][tour[j]] + dist[tour[i+1]][tour[k]] + dist[tour[j+1]][tour[(k+1)%n]];
                if (cost_3 - cost_0 < -1e-9) {
                    // Apply move 3
                    std::vector<int> new_tour = tour;
                    std::reverse(new_tour.begin() + i + 1, new_tour.begin() + j + 1);
                    std::reverse(new_tour.begin() + j + 1, new_tour.begin() + k + 1);
                    long long new_cost = calculate_tour_cost(dist, new_tour);
                    if (new_cost < current_cost) {
                        tour = new_tour;
                        current_cost = new_cost;
                        // Rebuild position array
                        for (int idx = 0; idx < n; idx++) {
                            pos[tour[idx]] = idx;
                        }
                        dont_look.assign(n, false);
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

long long lin_kernighan_tsp(const DistMatrix& dist, 
                            int max_iterations,
                            int random_restarts) {
    auto [tour, cost] = lin_kernighan_tsp_with_tour(dist, max_iterations, random_restarts);
    return cost;
}

std::pair<std::vector<int>, long long> lin_kernighan_tsp_with_tour(const DistMatrix& dist,
                                                                    int max_iterations,
                                                                    int random_restarts) {
    int n = dist.size();
    long long best_cost = std::numeric_limits<long long>::max();
    std::vector<int> best_tour;
    
    for (int restart = 0; restart < random_restarts; restart++) {
        std::vector<int> tour;
        if (restart == 0) {
            tour = greedy_nearest_neighbor(dist);
        } else {
            tour = random_tour(n);
        }
        
        long long current_cost = calculate_tour_cost(dist, tour);
        bool improving = true;
        int iteration = 0;
        
        // Build position array for faster lookups
        std::vector<int> pos(n);
        for (int i = 0; i < n; i++) {
            pos[tour[i]] = i;
        }
        
        // Don't-look bits for pruning
        std::vector<bool> dont_look(n, false);
        int no_improve_count = 0;
        
        // For large instances, limit search neighborhood
        int search_radius = n;
        if (n > 500) {
            search_radius = std::min(n, std::max(50, n / 10));
        }
        
        // 2-opt local search with don't-look bits and optional 3-opt
        while (improving && iteration < max_iterations) {
            iteration++;
            improving = false;
            
            for (int i = 0; i < n && !improving; i++) {
                if (dont_look[tour[i]]) continue;
                
                // Limited search radius for large instances
                for (int offset = 2; offset < search_radius; offset++) {
                    int j = (i + offset) % n;
                    if (j == (i + n - 1) % n) continue;
                    
                    long long delta = two_opt_delta(dist, tour, i, j);
                    if (delta < -1e-9) {
                        // Apply 2-opt move in-place
                        two_opt_move_inplace(tour, pos, i, j);
                        current_cost += delta;
                        improving = true;
                        
                        // Clear don't-look bits for affected nodes
                        dont_look[tour[i]] = false;
                        dont_look[tour[j]] = false;
                        dont_look[tour[(i + 1) % n]] = false;
                        dont_look[tour[(j + 1) % n]] = false;
                        
                        no_improve_count = 0;
                        break;
                    }
                }
            }
            
            // Try 3-opt if 2-opt didn't improve
            // if (!improving) {
            //     if (try_3opt_moves(dist, tour, pos, dont_look, current_cost, n)) {
            //         improving = true;
            //         no_improve_count = 0;
            //     }
            // }
            
            // Mark all as don't-look if no improvement
            if (!improving) {
                for (int i = 0; i < n; i++) {
                    dont_look[tour[i]] = true;
                }
                no_improve_count++;
                
                // Reset don't-look bits occasionally
                if (no_improve_count >= 3) {
                    std::fill(dont_look.begin(), dont_look.end(), false);
                    no_improve_count = 0;
                    improving = true;  // Try one more pass
                }
            }
        }
        
        if (current_cost < best_cost) {
            best_cost = current_cost;
            best_tour = tour;
        }
    }
    
    return {best_tour, best_cost};
}
