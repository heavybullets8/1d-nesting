#include "parse.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

// Helper to find the greatest common divisor for fraction simplification
static int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

/**
 * @brief Parses a string representing a fraction (e.g., "1/2") or a decimal
 * (e.g., "0.5").
 * @param s The input string.
 * @return The parsed value as a double. Returns 0.0 on failure.
 */
double parseFraction(const std::string &s) {
  std::string trimmed = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
  if (trimmed.empty())
    return 0.0;

  // Check for a fraction format like "numerator / denominator"
  if (trimmed.find('/') != std::string::npos) {
    std::regex re(R"(\s*(\d+(?:\.\d+)?)\s*\/\s*(\d+(?:\.\d+)?)\s*)");
    std::smatch matches;
    if (std::regex_match(trimmed, matches, re)) {
      try {
        double num = std::stod(matches[1].str());
        double den = std::stod(matches[2].str());
        if (den != 0) {
          return num / den;
        }
      } catch (const std::exception &) {
        return 0.0;
      }
    }
  } else {
    // If not a fraction, try to parse as a simple number
    try {
      size_t pos;
      double val = std::stod(trimmed, &pos);
      // Ensure the whole string was consumed to avoid partial matches
      if (pos == trimmed.length()) {
        return val;
      }
    } catch (const std::exception &) {
      return 0.0;
    }
  }
  return 0.0; // Return 0.0 on any parsing failure
}

/**
 * @brief Parses various length formats into inches as a double.
 *
 * Handles formats like:
 * - "24'" (24 feet)
 * - "7'6\"" (7 feet 6 inches)
 * - "7' 6\"" (7 feet 6 inches with space)
 * - "7' 6" (7 feet 6 inches without inch mark)
 * - "7' 6 1/2\"" (7 feet 6.5 inches)
 * - "7' 6\" 1/2" (alternative format)
 * - "110 1/8" (110 and 1/8 inches)
 * - "110.125" (decimal inches)
 *
 * @param s The input string.
 * @return The total length in inches as a double.
 */
double parseAdvancedLength(const std::string &s) {
  std::string input = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
  if (input.empty())
    return 0.0;

  double total_inches = 0.0;

  // First, check for feet marker (')
  size_t feet_pos = input.find('\'');
  if (feet_pos != std::string::npos) {
    // Parse feet part
    std::string feet_str = input.substr(0, feet_pos);
    total_inches += parseFraction(feet_str) * 12.0;

    // Get the remaining string after feet
    input = input.substr(feet_pos + 1);
    input = std::regex_replace(input, std::regex("^\\s+|\\s+$"), "");
  }

  // If nothing left after feet, we're done
  if (input.empty())
    return total_inches;

  // Now we need to parse the inches part, which might be in various formats:
  // - "6\"" (just inches)
  // - "6\" 1/2" (inches followed by fraction)
  // - "6 1/2\"" (mixed number with inch mark at end)
  // - "6 1/2" (mixed number without inch mark)
  // - "1/2" (just a fraction)

  // Remove any trailing inch mark first to simplify parsing
  bool had_inch_mark = false;
  if (!input.empty() && input.back() == '"') {
    input.pop_back();
    had_inch_mark = true;
    input = std::regex_replace(input, std::regex("^\\s+|\\s+$"), "");
  }

  // Check if there's an inch mark in the middle (like "6\" 1/2")
  size_t inch_mark_pos = input.find('"');
  if (inch_mark_pos != std::string::npos) {
    // Parse the part before the inch mark as whole inches
    std::string before_mark = input.substr(0, inch_mark_pos);
    total_inches += parseFraction(before_mark);

    // Parse the part after the inch mark as additional fraction
    std::string after_mark = input.substr(inch_mark_pos + 1);
    after_mark = std::regex_replace(after_mark, std::regex("^\\s+|\\s+$"), "");
    if (!after_mark.empty()) {
      total_inches += parseFraction(after_mark);
    }
  } else {
    // No inch mark in the middle, parse as mixed number or single value

    // Look for a space that might indicate a mixed number (like "6 1/2")
    // We need to be careful to identify the right space - the one before a
    // fraction
    size_t last_space = std::string::npos;
    for (size_t i = 0; i < input.length(); ++i) {
      if (std::isspace(input[i])) {
        // Check if what follows looks like a fraction
        size_t next_non_space = input.find_first_not_of(" \t", i);
        if (next_non_space != std::string::npos) {
          size_t slash_pos = input.find('/', next_non_space);
          if (slash_pos != std::string::npos) {
            // This space is before a fraction
            last_space = i;
          }
        }
      }
    }

    if (last_space != std::string::npos) {
      // Parse as mixed number
      std::string whole_part = input.substr(0, last_space);
      std::string frac_part = input.substr(last_space + 1);

      total_inches += parseFraction(whole_part);
      total_inches += parseFraction(frac_part);
    } else {
      // Parse as single value (either fraction or decimal)
      total_inches += parseFraction(input);
    }
  }

  return total_inches;
}

/**
 * @brief Formats a total number of inches into a human-readable string.
 * e.g., 100.5 becomes "8' 4 1/2\""
 * e.g., 288.0 becomes "24'"
 * @param total_inches The length in inches.
 * @return A formatted string.
 */
std::string prettyLen(double total_inches) {
  if (std::abs(total_inches) < 1.0 / 64.0)
    return "0\"";

  std::stringstream ss;
  if (total_inches < 0) {
    ss << "-";
    total_inches = -total_inches;
  }

  // Round to the nearest 1/32 for clean display
  total_inches = std::round(total_inches * 32.0) / 32.0;

  int feet = static_cast<int>(total_inches / 12.0);
  double remaining_inches = total_inches - (feet * 12.0);

  if (remaining_inches >= 12.0 - 1.0 / 64.0) {
    feet++;
    remaining_inches = 0;
  }

  int inches_whole = static_cast<int>(remaining_inches);
  double inches_fractional = remaining_inches - inches_whole;

  bool has_feet = feet > 0;
  bool has_inches = inches_whole > 0;
  bool has_fraction = inches_fractional > 1.0 / 64.0;

  if (has_feet) {
    ss << feet << "'";
  }

  if (has_feet && (has_inches || has_fraction)) {
    ss << " ";
  }

  if (has_inches) {
    ss << inches_whole;
  }

  if (has_fraction) {
    if (has_inches) {
      ss << " ";
    }
    int denominator = 32;
    int numerator =
        static_cast<int>(std::round(inches_fractional * denominator));
    if (numerator > 0) {
      int common = gcd(numerator, denominator);
      numerator /= common;
      denominator /= common;
      ss << numerator << "/" << denominator;
    }
  }

  if (!has_feet && !has_inches && !has_fraction) {
    return "0\"";
  }

  if (has_inches || has_fraction || !has_feet) {
    ss << "\"";
  }

  return ss.str();
}
