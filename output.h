#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"
#include <string>
#include <vector>

// Print results to console
void printResults(const std::string &tubing, int stockLen,
                  [[maybe_unused]] double kerf,
                  [[maybe_unused]] const std::vector<Cut> &cuts,
                  const Solution &solution);

// Generate HTML output file
void generateHTML(const std::string &filename, const std::string &tubing,
                  int stockLen, double kerf,
                  [[maybe_unused]] const std::vector<Cut> &cuts,
                  const Solution &solution);

// Group sticks into patterns for cleaner output
std::vector<Pattern> groupPatterns(const std::vector<Stick> &sticks);

// Open file in system's default browser
void openFile(const std::string &filename);

#endif // OUTPUT_H
