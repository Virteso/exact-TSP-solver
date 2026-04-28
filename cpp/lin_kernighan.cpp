#include "lin_kernighan.h"
#include <algorithm>
#include <random>
#include <limits>
#include <cstring>

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

long long two_opt_delta(const DistMatrix& dist, const std::vector<int>& tour, int i, int k) {
    int n = tour.size();
    long long old_cost = dist[tour[i]][tour[i + 1]] + dist[tour[k]][tour[(k + 1) % n]];
    long long new_cost = dist[tour[i]][tour[k]] + dist[tour[i + 1]][tour[(k + 1) % n]];
    return new_cost - old_cost;
}

long long lin_kernighan_tsp(const DistMatrix& dist, 
                            int max_iterations,
                            int random_restarts) {
    int n = dist.size();
    long long best_cost = std::numeric_limits<long long>::max();
    
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
        
        // 2-opt local search
        while (improving && iteration < max_iterations) {
            iteration++;
            improving = false;
            
            for (int i = 0; i < n && !improving; i++) {
                for (int j = i + 2; j < n; j++) {
                    if (j == n - 1 && i == 0) continue;
                    
                    long long delta = two_opt_delta(dist, tour, i, j);
                    if (delta < -1e-9) {
                        tour = two_opt_move(tour, i, j);
                        current_cost += delta;
                        improving = true;
                        break;
                    }
                }
            }
        }
        
        if (current_cost < best_cost) {
            best_cost = current_cost;
        }
    }
    
    return best_cost;
}
