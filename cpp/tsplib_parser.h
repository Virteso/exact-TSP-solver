#ifndef TSPLIB_PARSER_H
#define TSPLIB_PARSER_H

#include <vector>
#include <string>

using DistMatrix = std::vector<std::vector<int>>;

struct TSPProblem {
    std::string name;
    std::string type;
    std::string edge_weight_type;
    std::string edge_weight_format;
    int dimension;
    std::vector<std::pair<double, double>> coordinates;
    DistMatrix distances;
};

/**
 * Parse a TSP file in TSPLIB format
 * Supports: EXPLICIT, EUC_2D, EUC_3D, MAX_2D, MAN_2D, ATT norms
 * Supports: LOWER_DIAG_ROW, UPPER_ROW, FULL_MATRIX formats
 */
TSPProblem parse_tsp_file(const std::string& filename);

/**
 * Calculate Euclidean distance between two points
 */
int euclidean_distance(double x1, double y1, double x2, double y2);

/**
 * Calculate Euclidean distance between two points and round up
 */
int euclidean_ceil_distance(double x1, double y1, double x2, double y2);

/**
 * Calculate ATT distance (used in some TSPLIB instances)
 */
int att_distance(double x1, double y1, double x2, double y2);

/**
 * Calculate maximum norm (Chebyshev distance)
 */
int max_norm_distance(double x1, double y1, double x2, double y2);

/**
 * Calculate Manhattan distance
 */
int manhattan_distance(double x1, double y1, double x2, double y2);

/**
 * Build lookup table from problem
 */
DistMatrix build_lookup_table(const TSPProblem& problem);

#endif
