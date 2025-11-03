#include "algorithm.h"

#include <Highs.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

// Use a scaling factor to convert doubles to integers for the MIP solver,
// avoiding floating-point precision issues. A power of 2 like 1024 (2^10)
// is good for handling binary fractions like 1/16, 1/32, etc.
const int PRECISION_SCALE = 1024;

// Forward declaration for the internal pattern generation function
static std::vector<std::vector<long long>>
generatePatterns(const std::vector<long long>& availableCuts,
                 long long stockLen, long long kerf);

Solution optimizeCutting(const std::vector<Cut>& cuts, double stockLen,
                         double kerf) {
  // --- SCALING: Convert all double inputs to scaled integers ---
  long long scaled_stockLen =
      static_cast<long long>(std::round(stockLen * PRECISION_SCALE));
  long long scaled_kerf =
      static_cast<long long>(std::round(kerf * PRECISION_SCALE));

  // Create a list of all required cuts, scaled to integers
  std::vector<long long> allScaledCuts;
  for (const auto& cut : cuts) {
    allScaledCuts.push_back(
        static_cast<long long>(std::round(cut.length * PRECISION_SCALE)));
  }

  // Generate valid patterns using scaled integers
  auto patterns = generatePatterns(allScaledCuts, scaled_stockLen, scaled_kerf);
  if (patterns.empty()) {
    std::cerr << "Error: no valid cutting patterns could be generated. "
                 "Check if any cut is larger than the stock length."
              << std::endl;
    return Solution();
  }

  // Step 2: Build the Mixed-Integer Programming (MIP) model using HiGHS
  Highs highs;
  highs.setOptionValue("output_flag", false);
  HighsModel model;

  // Create a map to track the demand for each unique cut length
  std::unordered_map<long long, int> cutDemand;
  for (long long scaled_len : allScaledCuts) {
    cutDemand[scaled_len]++;
  }

  // Extract unique keys for consistent row ordering
  std::vector<long long> uniqueCutKeys;
  for (const auto& [len, demand] : cutDemand) {
    uniqueCutKeys.push_back(len);
  }
  std::sort(uniqueCutKeys.begin(), uniqueCutKeys.end());

  // Variables: one integer variable per pattern
  model.lp_.num_col_ = patterns.size();
  model.lp_.col_cost_.assign(patterns.size(),
                             1.0); // Objective: minimize # of sticks
  model.lp_.col_lower_.assign(patterns.size(), 0.0);
  model.lp_.col_upper_.assign(patterns.size(), kHighsInf);
  model.lp_.integrality_.assign(patterns.size(), HighsVarType::kInteger);

  // Constraints: one for each unique cut length required
  model.lp_.num_row_ = cutDemand.size();
  model.lp_.row_lower_.resize(cutDemand.size());
  model.lp_.row_upper_.resize(cutDemand.size());

  // Build the constraint matrix A (in column-wise format)
  std::vector<int> Astart = {0};
  std::vector<int> Aindex;
  std::vector<double> Avalue;

  for (const auto& pattern : patterns) {
    std::unordered_map<long long, int> patternCounts;
    for (long long piece : pattern) {
      patternCounts[piece]++;
    }

    for (size_t i = 0; i < uniqueCutKeys.size(); ++i) {
      long long cutLen = uniqueCutKeys[i];
      if (patternCounts.count(cutLen) > 0) {
        Aindex.push_back(i);
        Avalue.push_back(patternCounts[cutLen]);
      }
    }
    Astart.push_back(Aindex.size());
  }

  for (size_t i = 0; i < uniqueCutKeys.size(); ++i) {
    model.lp_.row_lower_[i] = cutDemand[uniqueCutKeys[i]];
    model.lp_.row_upper_[i] =
        cutDemand[uniqueCutKeys[i]]; // Enforce exact quantity
  }

  model.lp_.a_matrix_.format_ = MatrixFormat::kColwise;
  model.lp_.a_matrix_.start_ = Astart;
  model.lp_.a_matrix_.index_ = Aindex;
  model.lp_.a_matrix_.value_ = Avalue;
  model.lp_.sense_ = ObjSense::kMinimize;

  // Step 3: Solve the MIP model with HiGHS
  highs.passModel(model);
  highs.run();

  if (highs.getModelStatus() != HighsModelStatus::kOptimal) {
    std::cerr << "HiGHS could not find an optimal solution. Status: "
              << highs.modelStatusToString(highs.getModelStatus()) << std::endl;
    return Solution();
  }

  // Step 4: Convert the solver output, scaling back to doubles
  const HighsSolution& solution = highs.getSolution();
  Solution result;
  double totalUsedLengthPrecise = 0.0;

  for (size_t i = 0; i < patterns.size(); i++) {
    int numSticks = static_cast<int>(std::round(solution.col_value[i]));
    if (numSticks == 0)
      continue;

    const auto& pattern = patterns[i];
    double preciseUsedLen = 0.0;

    std::vector<Cut> cutSlice;
    for (long long scaled_len : pattern) {
      double len = static_cast<double>(scaled_len) / PRECISION_SCALE;
      cutSlice.push_back(Cut(len, 0));
      preciseUsedLen += len;
    }
    // For n pieces, we need n-1 kerfs (between pieces, not after the last one)
    if (cutSlice.size() > 0) {
      preciseUsedLen += (cutSlice.size() - 1) * kerf;
    }

    for (int s = 0; s < numSticks; s++) {
      Stick stick;
      stick.cuts = cutSlice;
      stick.stock_len = stockLen;
      stick.used_len = preciseUsedLen;
      stick.waste_len = stockLen - preciseUsedLen;
      result.sticks.push_back(stick);
    }
    totalUsedLengthPrecise += preciseUsedLen * numSticks;
  }

  result.num_sticks = result.sticks.size();
  result.total_waste = result.num_sticks * stockLen - totalUsedLengthPrecise;

  return result;
}

/**
 * @brief Generates all possible cutting patterns using scaled integers.
 *
 * This function recursively finds all combinations of cuts that can fit onto a
 * single stock piece.  
 * */
static std::vector<std::vector<long long>>
generatePatterns(const std::vector<long long>& availableCuts,
                 long long stockLen, long long kerf) {
  std::set<long long> uniqueCutsSet(availableCuts.begin(), availableCuts.end());
  std::vector<long long> uniqueCuts(uniqueCutsSet.begin(), uniqueCutsSet.end());
  std::sort(uniqueCuts.rbegin(), uniqueCuts.rend());

  std::vector<std::vector<long long>> patterns;
  std::vector<long long> currentPattern;

  std::function<void(int, long long)> findPatterns =
      [&](int startIndex, long long remainingLen) {
        // Add the current combination as a valid pattern *before* trying to add
        // more
        if (!currentPattern.empty()) {
          patterns.push_back(currentPattern);
        }

        // Try adding more cuts, starting from `startIndex` to avoid
        // permutations
        for (size_t i = startIndex; i < uniqueCuts.size(); i++) {
          long long cutLength = uniqueCuts[i];
          // For n pieces, we need n-1 kerfs (between pieces, not after the last one)
          // First cut: no kerf needed yet. Subsequent cuts: need kerf + cut length
          long long requiredSpace = cutLength + (currentPattern.empty() ? 0 : kerf);
          if (remainingLen >= requiredSpace) {
            currentPattern.push_back(cutLength);
            findPatterns(i, remainingLen - requiredSpace);
            currentPattern.pop_back(); // Backtrack for the next combination
          }
        }
      };

  // Initial call to start the recursion.
  findPatterns(0, stockLen);

  // Remove duplicate patterns
  for (auto& p : patterns) {
    std::sort(p.begin(), p.end());
  }
  std::sort(patterns.begin(), patterns.end());
  patterns.erase(std::unique(patterns.begin(), patterns.end()), patterns.end());

  return patterns;
}
