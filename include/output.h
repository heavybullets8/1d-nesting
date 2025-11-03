#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"

#include <string>
#include <vector>

// Group sticks into patterns for cleaner output.
// This function is used by web_server.cpp to prepare data for the API response.
std::vector<Pattern> groupPatterns(const std::vector<Stick>& sticks);

// Convert a decimal to a fraction string for display.
// This function is used by web_server.cpp to prepare data for the API response.
std::string toFraction(double value);

#endif // OUTPUT_H
