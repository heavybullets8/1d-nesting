#ifndef TYPES_H
#define TYPES_H

#include <vector>

// Cut represents a single cut piece
struct Cut {
  double length{0.0}; // in inches
  int id{0};

  Cut() = default;
  Cut(double len, int id_) : length(len), id(id_) {}
};

// Stick represents a stock piece with its cuts
struct Stick {
  std::vector<Cut> cuts;
  double stock_len{0.0}; // in inches
  double used_len{0.0};  // in inches
  double waste_len{0.0}; // in inches
};

// Solution represents a cutting solution
struct Solution {
  std::vector<Stick> sticks;
  double total_waste{0.0}; // in inches
  int num_sticks{0};
};

// Pattern for grouping identical cutting patterns
struct Pattern {
  std::vector<Cut> cuts;
  int count{0};
  double used_len{0.0};
  double waste_len{0.0};
};

#endif // TYPES_H
