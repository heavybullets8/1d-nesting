#include "algorithm.h"
#ifndef NO_HIGHS
#include <Highs.h>
#endif
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <unordered_map>

// Forward declaration
std::vector<std::vector<int>>
generatePatterns(const std::vector<int> &availableCuts, int stockLen,
                 double kerf);

#ifndef NO_HIGHS
Solution optimizeCutting(const std::vector<Cut> &cuts, int stockLen,
                         double kerf) {
  // Step 1: Generate all possible cutting patterns
  std::vector<int> allCuts;
  for (const auto &cut : cuts) {
    allCuts.push_back(cut.length);
  }

  // Generate valid patterns considering kerf for every cut
  auto patterns = generatePatterns(allCuts, stockLen, kerf);
  if (patterns.empty()) {
    std::cerr << "Error: no valid cutting patterns could be generated. "
                 "Check if any cut is larger than the stock length."
              << std::endl;
    return Solution();
  }

  // Step 2: Build the Mixed-Integer Programming (MIP) model using HiGHS
  Highs highs;
  HighsModel model;

  // Variables: one integer variable per pattern, representing the number of
  // times that pattern (stick) is used.
  model.lp_.num_col_ = patterns.size();
  model.lp_.col_cost_.resize(patterns.size(),
                             1.0); // Objective: minimize # of sticks
  model.lp_.col_lower_.resize(patterns.size(), 0.0);
  model.lp_.col_upper_.resize(patterns.size(), kHighsInf);

  // All variables must be integers
  model.lp_.integrality_.resize(patterns.size(), HighsVarType::kInteger);

  // Create a map to track the required demand for each unique cut length
  std::unordered_map<int, int> cutDemand;
  for (const auto &cut : cuts) {
    cutDemand[cut.length]++;
  }

  model.lp_.num_row_ = cutDemand.size();
  model.lp_.row_lower_.resize(cutDemand.size());
  model.lp_.row_upper_.resize(
      cutDemand.size()); // Upper bound will be set to demand

  // Build the constraint matrix A (in column-wise format)
  std::vector<int> Astart;
  std::vector<int> Aindex;
  std::vector<double> Avalue;

  Astart.push_back(0);

  // For each pattern (column in the matrix)
  for (size_t i = 0; i < patterns.size(); i++) {
    // Count how many of each cut length this pattern produces
    std::unordered_map<int, int> patternCounts;
    for (int piece : patterns[i]) {
      patternCounts[piece]++;
    }

    // Add non-zero entries to the constraint matrix for this pattern
    int row = 0;
    for (const auto &[cutLen, demand] : cutDemand) {
      if (patternCounts.count(cutLen) && patternCounts[cutLen] > 0) {
        Aindex.push_back(row);
        Avalue.push_back(patternCounts[cutLen]);
      }
      row++;
    }
    Astart.push_back(Aindex.size());
  }

  int row = 0;
  for (const auto &[cutLen, demand] : cutDemand) {
    model.lp_.row_lower_[row] = demand;
    model.lp_.row_upper_[row] = demand; // Enforces exact quantity
    row++;
  }

  // Assign the matrix to the model
  model.lp_.a_matrix_.format_ = MatrixFormat::kColwise;
  model.lp_.a_matrix_.start_ = Astart;
  model.lp_.a_matrix_.index_ = Aindex;
  model.lp_.a_matrix_.value_ = Avalue;

  // Set the objective sense to minimization
  model.lp_.sense_ = ObjSense::kMinimize;

  // Step 3: Solve the MIP model with HiGHS
  highs.passModel(model);
  highs.setOptionValue("presolve", "on");
  highs.setOptionValue("mip_rel_gap", 0.0);

  HighsStatus status = highs.run();

  // If no solution is found, it might be impossible to create the exact
  // quantities. This can happen with certain combinations of cuts and stock
  // length.
  if (status != HighsStatus::kOk) {
    std::cerr << "HiGHS solve error: "
              << highs.modelStatusToString(highs.getModelStatus()) << std::endl;
    std::cerr << "It might be impossible to generate the exact number of "
                 "requested parts with the given stock length."
              << std::endl;
    return Solution();
  }

  const HighsSolution &solution = highs.getSolution();

  // Step 4: Convert the solver output into our domain-specific structs
  Solution result;
  double totalUsedLengthPrecise = 0.0;

  for (size_t i = 0; i < patterns.size(); i++) {
    int numSticks = static_cast<int>(std::round(solution.col_value[i]));
    if (numSticks == 0)
      continue;

    // For each stick of this pattern, calculate its properties
    std::vector<Cut> cutSlice;
    double preciseUsedLen = 0.0;

    for (int cl : patterns[i]) {
      cutSlice.push_back(Cut(cl, 0));
      preciseUsedLen += cl;
    }

    // Correctly account for kerf for EACH cut in the pattern
    preciseUsedLen += cutSlice.size() * kerf;
    int usedLen = static_cast<int>(std::round(preciseUsedLen));

    // Create stick objects for the solution
    for (int s = 0; s < numSticks; s++) {
      Stick stick;
      stick.cuts = cutSlice;
      stick.stock_len = stockLen;
      stick.used_len = usedLen;
      stick.waste_len = stockLen - usedLen;
      result.sticks.push_back(stick);
    }

    totalUsedLengthPrecise += preciseUsedLen * numSticks;
  }

  result.num_sticks = result.sticks.size();
  result.total_waste = static_cast<int>(
      std::round(result.num_sticks * stockLen - totalUsedLengthPrecise));

  return result;
}
#endif // NO_HIGHS

/**
 * @brief Generates all possible unique cutting patterns for a single stick.
 *
 * This function recursively finds all combinations of cuts that can fit onto a
 * single stock piece, correctly accounting for kerf with each cut.
 *
 * @param availableCuts A list of all cut lengths that are needed.
 * @param stockLen The length of the stock material.
 * @param kerf The width of the saw blade.
 * @return A vector of patterns, where each pattern is a vector of cut lengths.
 */
std::vector<std::vector<int>>
generatePatterns(const std::vector<int> &availableCuts, int stockLen,
                 double kerf) {
  // Get unique cut lengths and sort them descending. This is a heuristic to
  // find patterns with larger pieces first, which can be more efficient.
  std::set<int> uniqueCutsSet(availableCuts.begin(), availableCuts.end());
  std::vector<int> uniqueCuts(uniqueCutsSet.begin(), uniqueCutsSet.end());
  std::sort(uniqueCuts.rbegin(), uniqueCuts.rend());

  std::vector<std::vector<int>> patterns;
  std::vector<int> currentPattern;

  // We use `double` for remaining length to handle fractional kerf precisely.
  std::function<void(int, double)> findPatterns = [&](int startIndex,
                                                      double remainingLen) {
    bool canAddMore = false;
    // Try adding more cuts, starting from `startIndex` to generate combinations
    // instead of permutations.
    for (int i = startIndex; i < static_cast<int>(uniqueCuts.size()); i++) {
      double cutLength = static_cast<double>(uniqueCuts[i]);

      // A cut consumes the length of the piece PLUS the blade's kerf.
      if (remainingLen >= cutLength + kerf) {
        canAddMore = true;
        currentPattern.push_back(uniqueCuts[i]);
        findPatterns(i, remainingLen - (cutLength + kerf));
        currentPattern.pop_back(); // Backtrack for the next combination
      }
    }

    // If no more pieces can be added from the current state, this path is
    // complete. If the pattern is not empty, it's a valid maximal pattern.
    if (!canAddMore && !currentPattern.empty()) {
      patterns.push_back(currentPattern);
    }
  };

  // Initial call to start the recursion.
  findPatterns(0, static_cast<double>(stockLen));

  // The recursive search might produce the same set of cuts from different
  // paths. To ensure the final list is unique, we sort each pattern
  // internally, then sort the list of patterns and remove duplicates.
  for (auto &p : patterns) {
    std::sort(p.begin(), p.end());
  }
  std::sort(patterns.begin(), patterns.end());
  patterns.erase(std::unique(patterns.begin(), patterns.end()), patterns.end());

  return patterns;
}
