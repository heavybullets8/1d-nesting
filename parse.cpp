#include "parse.h" // Assuming this contains Cut struct and function declarations
#include <cmath>
#include <regex>

/**
 * @brief Parses a string representing a fraction (e.g., "1/2") or a decimal
 * (e.g., "0.5").
 * * @param s The input string.
 * @return The parsed value as a double. Returns 0.0 on failure.
 */
double parseFraction(const std::string &s) {
  std::string trimmed = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");

  // Check for a fraction format like "numerator / denominator"
  if (trimmed.find('/') != std::string::npos) {
    std::regex re(R"((\d+(?:\.\d+)?)\s*\/\s*(\d+(?:\.\d+)?))");
    std::smatch matches;
    if (std::regex_match(trimmed, matches, re)) {
      double num = std::stod(matches[1].str());
      double den = std::stod(matches[2].str());
      if (den != 0) {
        return num / den;
      }
    }
  } else {
    // If not a fraction, try to parse as a simple decimal
    try {
      return std::stod(trimmed);
    } catch (const std::invalid_argument &) {
      // Not a valid number
    }
  }
  return 0.0;
}

/**
 * @brief Parses various length formats into inches.
 *
 * This function can handle:
 * - Simple inches: "288"
 * - Feet notation: "24'"
 * - Feet and inches: "7'6\""
 * - Inches with fractions: "6 1/2"
 *
 * @param s The input string.
 * @return The total length in inches, rounded to the nearest whole number.
 */
int parseAdvancedLength(const std::string &s) {
  std::string trimmed = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");

  // First, check for a simple integer string. This is crucial to prevent
  // std::stoi from partially parsing a string like "24'" and incorrectly
  // returning 24.
  try {
    size_t pos;
    int val = std::stoi(trimmed, &pos);
    if (pos == trimmed.length()) {
      return val; // The entire string is a valid integer.
    }
  } catch (const std::exception &) {
    // Not a simple integer, so proceed to more complex parsing.
  }

  double total_inches = 0;
  std::string remaining = trimmed;

  // 1. Handle feet notation (e.g., "24'")
  size_t feet_pos = remaining.find('\'');
  if (feet_pos != std::string::npos) {
    total_inches += std::stod(remaining.substr(0, feet_pos)) * 12.0;
    remaining = remaining.substr(feet_pos + 1);
    remaining = std::regex_replace(remaining, std::regex("^\\s+|\\s+$"), "");
  }

  // 2. Handle inches and fractions from the remainder of the string
  if (!remaining.empty()) {
    // Remove trailing double-quote if present
    if (remaining.back() == '"') {
      remaining.pop_back();
    }
    remaining = std::regex_replace(remaining, std::regex("^\\s+|\\s+$"), "");

    if (!remaining.empty()) {
      // Check for a mixed number format like "6 1/2"
      std::regex re(R"((\d+)\s+(\d+\s*\/\s*\d+))");
      std::smatch matches;
      if (std::regex_match(remaining, matches, re)) {
        total_inches += std::stod(matches[1].str());
        total_inches += parseFraction(matches[2].str());
      } else {
        // Otherwise, the remainder is either just inches ("6") or a fraction
        // ("1/2")
        total_inches += parseFraction(remaining);
      }
    }
  }

  return static_cast<int>(round(total_inches));
}
