#include "tsplib_parser.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

// Helper function to trim whitespace
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// Helper function to convert string to uppercase
static std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

int euclidean_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return static_cast<int>(sqrt(dx * dx + dy * dy) + 0.5);
}

int euclidean_ceil_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return static_cast<int>(ceil(sqrt(dx * dx + dy * dy)));
}

int att_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    double rij = sqrt((dx * dx + dy * dy) / 10.0);
    int tij = static_cast<int>(rij);

    if (tij < rij)
        return tij + 1;

    return tij;
}

int max_norm_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    return static_cast<int>(dx < dy ? dy + 0.5 : dx + 0.5);
}

int manhattan_distance(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    return static_cast<int>(dx + dy + 0.5);
}

// GEO distance (Great Circle Distance on Earth for lat/lon coordinates)
// Using the formula from Concorde implementation
#define RRR (6378.388) // Earth radius in km
#define GH_PI (3.141592)
int geo_distance(double lat1, double lon1, double lat2, double lon2) {
    double deg, min;
    double lat1_rad, lat2_rad, lon1_rad, lon2_rad;
    double q1, q2, q3;
    int dd;

    deg = static_cast<double>(static_cast<int>(lat1));
    min = lat1 - deg;
    lat1_rad = GH_PI * (deg + 5.0 * min / 3.0) / 180.0;
    deg = static_cast<double>(static_cast<int>(lat2));
    min = lat2 - deg;
    lat2_rad = GH_PI * (deg + 5.0 * min / 3.0) / 180.0;

    deg = static_cast<double>(static_cast<int>(lon1));
    min = lon1 - deg;
    lon1_rad = GH_PI * (deg + 5.0 * min / 3.0) / 180.0;
    deg = static_cast<double>(static_cast<int>(lon2));
    min = lon2 - deg;
    lon2_rad = GH_PI * (deg + 5.0 * min / 3.0) / 180.0;

    q1 = cos (lon1_rad - lon2_rad);
    q2 = cos (lat1_rad - lat2_rad);
    q3 = cos (lat1_rad + lat2_rad);
    dd = 1 + static_cast<int>(RRR * acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)));
    return dd;
}

TSPProblem parse_tsp_file(const std::string& filename) {
    TSPProblem problem;
    problem.name = filename;
    problem.dimension = 0;
    problem.type = "";
    problem.edge_weight_type = "";
    problem.edge_weight_format = "LOWER_DIAG_ROW";  // Default format
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::string line;
    std::vector<int> edge_weights;
    bool reading_nodes = false;
    bool reading_edges = false;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#') continue;
        
        // Replace colons with spaces for easier parsing
        for (size_t i = 0; i < line.length(); i++) {
            if (line[i] == ':') line[i] = ' ';
        }
        
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key.empty()) continue;
        
        key = to_upper(key);
        
        // Parse NAME
        if (key == "NAME") {
            std::string rest;
            if (std::getline(iss, rest)) {
                problem.name = trim(rest);
            }
        }
        
        // Parse TYPE
        else if (key == "TYPE") {
            std::string type_val;
            iss >> type_val;
            problem.type = to_upper(type_val);
        }
        
        // Parse DIMENSION
        else if (key == "DIMENSION") {
            iss >> problem.dimension;
            problem.coordinates.resize(problem.dimension);
        }
        
        // Parse EDGE_WEIGHT_TYPE
        else if (key == "EDGE_WEIGHT_TYPE") {
            std::string weight_type;
            iss >> weight_type;
            problem.edge_weight_type = to_upper(weight_type);
        }
        
        // Parse EDGE_WEIGHT_FORMAT
        else if (key == "EDGE_WEIGHT_FORMAT") {
            std::string weight_format;
            iss >> weight_format;
            problem.edge_weight_format = to_upper(weight_format);
        }
        
        // NODE_COORD_SECTION
        else if (key == "NODE_COORD_SECTION") {
            reading_nodes = true;
            reading_edges = false;
            continue;
        }
        
        // EDGE_WEIGHT_SECTION
        else if (key == "EDGE_WEIGHT_SECTION") {
            reading_edges = true;
            reading_nodes = false;
            continue;
        }
        
        // EOF marker
        else if (key == "EOF") {
            break;
        }
        
        // Parse node coordinates
        if (reading_nodes && !key.empty() && std::isdigit(key[0])) {
            int node_id;
            double x, y;
            std::istringstream coord_iss(line);
            if (coord_iss >> node_id >> x >> y) {
                if (node_id > 0 && node_id <= problem.dimension) {
                    problem.coordinates[node_id - 1] = {x, y};
                }
            }
        }
        
        // Parse edge weights
        if (reading_edges && !key.empty() && std::isdigit(key[0])) {
            std::istringstream weight_iss(line);
            int weight;
            while (weight_iss >> weight) {
                edge_weights.push_back(weight);
            }
        }
    }
    
    file.close();
    
    // Build distance matrix
    if (problem.dimension <= 0) {
        throw std::runtime_error("Invalid dimension in TSP file: " + filename);
    }
    
    problem.distances.assign(problem.dimension, 
                            std::vector<int>(problem.dimension, 0));
    
    // If we have explicit edge weights
    if (!edge_weights.empty()) {
        int idx = 0;
        
        if (problem.edge_weight_format == "LOWER_DIAG_ROW") {
            // Lower triangular including diagonal
            for (int i = 0; i < problem.dimension; i++) {
                for (int j = 0; j <= i; j++) {
                    if (idx < (int)edge_weights.size()) {
                        int weight = edge_weights[idx++];
                        problem.distances[i][j] = weight;
                        if (i != j) problem.distances[j][i] = weight;
                    }
                }
            }
        } else if (problem.edge_weight_format == "UPPER_ROW") {
            // Upper triangular
            for (int i = 0; i < problem.dimension; i++) {
                for (int j = i + 1; j < problem.dimension; j++) {
                    if (idx < (int)edge_weights.size()) {
                        int weight = edge_weights[idx++];
                        problem.distances[i][j] = weight;
                        problem.distances[j][i] = weight;
                    }
                }
            }
        } else if (problem.edge_weight_format == "FULL_MATRIX") {
            // Full matrix
            for (int i = 0; i < problem.dimension; i++) {
                for (int j = 0; j < problem.dimension; j++) {
                    if (idx < (int)edge_weights.size()) {
                        problem.distances[i][j] = edge_weights[idx++];
                    }
                }
            }
        } else if (problem.edge_weight_format == "UPPER_DIAG_ROW") {
            // Upper triangular including diagonal
            for (int i = 0; i < problem.dimension; i++) {
                for (int j = i; j < problem.dimension; j++) {
                    if (idx < (int)edge_weights.size()) {
                        int weight = edge_weights[idx++];
                        problem.distances[i][j] = weight;
                        if (i != j) problem.distances[j][i] = weight;
                    }
                }
            }
        }
    } 
    // If we have coordinates, compute distances
    else if (!problem.coordinates.empty() && problem.coordinates[0].first > -1e9) {
        for (int i = 0; i < problem.dimension; i++) {
            for (int j = 0; j < problem.dimension; j++) {
                if (i == j) {
                    problem.distances[i][j] = 0;
                } else {
                    int dist = 0;
                    if (problem.edge_weight_type == "EUC_2D") {
                        dist = euclidean_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else if (problem.edge_weight_type == "ATT") {
                        dist = att_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else if (problem.edge_weight_type == "MAX_2D") {
                        dist = max_norm_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else if (problem.edge_weight_type == "MAN_2D") {
                        dist = manhattan_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else if (problem.edge_weight_type == "CEIL_2D") {
                        dist = euclidean_ceil_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else if (problem.edge_weight_type == "GEO") {
                        dist = geo_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    } else {
                        // Default to Euclidean
                        dist = euclidean_distance(
                            problem.coordinates[i].first,
                            problem.coordinates[i].second,
                            problem.coordinates[j].first,
                            problem.coordinates[j].second
                        );
                    }
                    problem.distances[i][j] = dist;
                }
            }
        }
    }
    
    return problem;
}

DistMatrix build_lookup_table(const TSPProblem& problem) {
    return problem.distances;
}
