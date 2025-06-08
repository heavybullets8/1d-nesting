#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "types.h"
#include <vector>

// Main optimization function using HiGHS
Solution optimizeCutting(const std::vector<Cut> &cuts, int stockLen,
                         double kerf);

// Generate all possible cutting patterns for a single stick
std::vector<std::vector<int>>
generatePatterns(const std::vector<int> &availableCuts, int stockLen,
                 double kerf);

#endif // ALGORITHM_H
