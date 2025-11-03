#include "output.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

/**
 * @brief Converts a decimal value into a simplified fraction string.
 *
 * @param value The decimal to convert (e.g., 0.125).
 * @return A string representing the fraction (e.g., "1/8").
 */
std::string toFraction(double value) {
  if (value == 0)
    return "0";

  const double tolerance = 1.0 / 64.0;
  // Common denominators for imperial measurements up to 1/32
  const int denominators[] = {2, 4, 8, 16, 32};

  for (int d : denominators) {
    int n = static_cast<int>(round(value * d));
    if (std::abs(value - static_cast<double>(n) / d) < tolerance) {
      int common = gcd(n, d);
      n /= common;
      d /= common;
      if (d == 1)
        return std::to_string(n);
      return std::to_string(n) + "/" + std::to_string(d);
    }
  }

  // Fallback for non-standard fractions
  std::stringstream ss;
  ss << std::fixed << std::setprecision(3) << value;
  return ss.str();
}

/**
 * @brief Helper function to group identical sticks into patterns for cleaner
 * output. This is used by the web_server to format the solution for JSON
 * response.
 *
 * @param sticks A vector of Stick objects representing the cutting solution.
 * @return A vector of Pattern objects, where identical sticks are grouped.
 */
std::vector<Pattern> groupPatterns(const std::vector<Stick>& sticks) {
  std::map<std::string, Pattern> patternMap;

  for (const auto& stick : sticks) {
    std::vector<double> lengths;
    for (const auto& cut : stick.cuts) {
      lengths.push_back(cut.length);
    }
    std::sort(lengths.begin(), lengths.end(), std::greater<double>());

    std::stringstream key_ss;
    for (size_t i = 0; i < lengths.size(); i++) {
      if (i > 0)
        key_ss << ",";
      // Use fixed precision for key to avoid floating point inconsistencies
      key_ss << std::fixed << std::setprecision(5) << lengths[i];
    }
    std::string key = key_ss.str();

    auto it = patternMap.find(key);
    if (it != patternMap.end()) {
      it->second.count++;
    } else {
      Pattern p;
      p.cuts = stick.cuts;
      std::sort(p.cuts.begin(), p.cuts.end(),
                [](const Cut& a, const Cut& b) { return a.length > b.length; });
      p.count = 1;
      p.used_len = stick.used_len;
      p.waste_len = stick.waste_len;
      patternMap[key] = p;
    }
  }

  std::vector<Pattern> patterns;
  for (const auto& [key, p] : patternMap) {
    patterns.push_back(p);
  }

  std::sort(patterns.begin(), patterns.end(),
            [](const Pattern& a, const Pattern& b) {
              if (a.count != b.count) {
                return a.count > b.count;
              }
              return a.used_len > b.used_len;
            });

  return patterns;
}
