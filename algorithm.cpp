#include "algorithm.h"
#include <Highs.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <unordered_map>

Solution optimizeCutting(const std::vector<Cut> &cuts, int stockLen,
                         double kerf) {
  // Step 1: Generate all possible cutting patterns
  std::vector<int> allCuts;
  for (const auto &cut : cuts) {
    allCuts.push_back(cut.length);
  }

  auto patterns = generatePatterns(allCuts, stockLen, kerf);
  if (patterns.empty()) {
    std::cerr << "Error: no valid cutting patterns could be generated."
              << std::endl;
    return Solution();
  }

  // Step 2: Build the MIP model using HiGHS
  Highs highs;
  HighsModel model;

  // Variables: one integer variable per pattern (number of sticks using that
  // pattern)
  model.lp_.num_col_ = patterns.size();
  model.lp_.col_cost_.resize(patterns.size(), 1.0); // Minimize number of sticks
  model.lp_.col_lower_.resize(patterns.size(), 0.0);
  model.lp_.col_upper_.resize(patterns.size(), kHighsInf);

  // All variables are integers
  model.lp_.integrality_.resize(patterns.size(), HighsVarType::kInteger);

  // Track demand for each cut length
  std::unordered_map<int, int> cutDemand;
  for (const auto &cut : cuts) {
    cutDemand[cut.length]++;
  }

  // Constraints: one per unique cut length
  model.lp_.num_row_ = cutDemand.size();
  model.lp_.row_lower_.resize(cutDemand.size());
  model.lp_.row_upper_.resize(cutDemand.size(), kHighsInf);

  // Build constraint matrix
  std::vector<int> Astart;
  std::vector<int> Aindex;
  std::vector<double> Avalue;

  Astart.push_back(0);

  // For each pattern (column)
  for (size_t i = 0; i < patterns.size(); i++) {
    // Count how many of each cut length this pattern provides
    std::unordered_map<int, int> patternCounts;
    for (int piece : patterns[i]) {
      patternCounts[piece]++;
    }

    // Add non-zero entries to the constraint matrix
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

  // Set constraint bounds (>= demand)
  int row = 0;
  for (const auto &[cutLen, demand] : cutDemand) {
    model.lp_.row_lower_[row] = demand;
    row++;
  }

  // Set the constraint matrix
  model.lp_.a_matrix_.format_ = MatrixFormat::kColwise;
  model.lp_.a_matrix_.start_ = Astart;
  model.lp_.a_matrix_.index_ = Aindex;
  model.lp_.a_matrix_.value_ = Avalue;

  // Set model sense (minimization)
  model.lp_.sense_ = ObjSense::kMinimize;

  // Step 3: Solve with HiGHS
  highs.passModel(model);

  // Configure solver options
  highs.setOptionValue("presolve", "on");
  highs.setOptionValue("mip_rel_gap", 0.01); // 1% optimality gap

  HighsStatus status = highs.run();

  if (status != HighsStatus::kOk) {
    std::cerr << "HiGHS solve error: "
              << highs.modelStatusToString(highs.getModelStatus()) << std::endl;
    return Solution();
  }

  const HighsSolution &solution = highs.getSolution();

  // Step 4: Convert solver output to our domain structs
  Solution result;
  int totalUsed = 0;

  for (size_t i = 0; i < patterns.size(); i++) {
    int numSticks = static_cast<int>(std::round(solution.col_value[i]));
    if (numSticks == 0)
      continue;

    // Create cuts for this pattern
    std::vector<Cut> cutSlice;
    int usedLen = 0;

    for (int cl : patterns[i]) {
      cutSlice.push_back(Cut(cl, 0));
      usedLen += cl;
    }

    // Add kerf
    if (cutSlice.size() > 1) {
      usedLen += static_cast<int>(std::round((cutSlice.size() - 1) * kerf));
    }

    // Create sticks using this pattern
    for (int s = 0; s < numSticks; s++) {
      Stick stick;
      stick.cuts = cutSlice;
      stick.stock_len = stockLen;
      stick.used_len = usedLen;
      stick.waste_len = stockLen - usedLen;
      result.sticks.push_back(stick);
    }

    totalUsed += usedLen * numSticks;
  }

  result.num_sticks = result.sticks.size();
  result.total_waste = result.num_sticks * stockLen - totalUsed;

  return result;
}

std::vector<std::vector<int>>
generatePatterns(const std::vector<int> &availableCuts, int stockLen,
                 double kerf) {
  // Get unique cut lengths
  std::set<int> uniqueCutsSet(availableCuts.begin(), availableCuts.end());
  std::vector<int> uniqueCuts(uniqueCutsSet.begin(), uniqueCutsSet.end());

  std::vector<std::vector<int>> patterns;
  std::vector<int> currentPattern;
  int kerfInt = static_cast<int>(std::round(kerf));

  // Recursive function to find all valid patterns
  std::function<void(int, int)> findPatterns = [&](int startIndex,
                                                   int remainingLen) {
    // Save current pattern if it has cuts
    if (!currentPattern.empty()) {
      patterns.push_back(currentPattern);
    }

    // Try adding more cuts
    for (int i = startIndex; i < static_cast<int>(uniqueCuts.size()); i++) {
      int cut = uniqueCuts[i];
      int kerfCost = currentPattern.empty() ? 0 : kerfInt;

      if (remainingLen >= cut + kerfCost) {
        currentPattern.push_back(cut);
        findPatterns(i, remainingLen - (cut + kerfCost));
        currentPattern.pop_back();
      }
    }
  };

  findPatterns(0, stockLen);

  return patterns;
}
