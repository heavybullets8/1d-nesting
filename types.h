#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>

// Cut represents a single cut piece
struct Cut {
  double length; // in inches
  int id;

  Cut(double len = 0.0, int id_ = 0) : length(len), id(id_) {}
};

// Stick represents a stock piece with its cuts
struct Stick {
  std::vector<Cut> cuts;
  double stock_len; // in inches
  double used_len;  // in inches
  double waste_len; // in inches

  Stick() : stock_len(0.0), used_len(0.0), waste_len(0.0) {}
};

// Solution represents a cutting solution
struct Solution {
  std::vector<Stick> sticks;
  double total_waste; // in inches
  int num_sticks;

  Solution() : total_waste(0.0), num_sticks(0) {}
};

// Pattern for grouping identical cutting patterns
struct Pattern {
  std::vector<Cut> cuts;
  int count;
  double used_len;
  double waste_len;

  Pattern() : count(0), used_len(0.0), waste_len(0.0) {}
};

#endif // TYPES_H
