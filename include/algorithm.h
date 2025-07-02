#ifndef ALGORITHM_H
#define ALGORITHM_H

#include "types.h"
#include <vector>

// Main optimization function using HiGHS
Solution optimizeCutting(const std::vector<Cut> &cuts, double stockLen,
                         double kerf);

#endif // ALGORITHM_H
