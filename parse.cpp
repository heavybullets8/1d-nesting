#include "parse.h"
#include <cmath>
#include <iostream>
#include <numeric>
#include <regex>
#include <sstream>
#include <string>

// Helper to find the greatest common divisor for fraction simplification
// Made static to prevent linker errors from multiple definitions.
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
 * Handles formats like: "24'", "7'6\"", "110 1/8", "110.125"
 * @param s The input string.
 * @return The total length in inches as a double.
 */
double parseAdvancedLength(const std::string &s) {
  std::string trimmed = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
  if (trimmed.empty())
    return 0.0;

  double total_inches = 0.0;

  // Handle feet (') and inches (") markers
  size_t feet_pos = trimmed.find('\'');
  if (feet_pos != std::string::npos) {
    total_inches += parseFraction(trimmed.substr(0, feet_pos)) * 12.0;
    trimmed = trimmed.substr(feet_pos + 1);
  }

  if (!trimmed.empty() && trimmed.back() == '"') {
    trimmed.pop_back();
  }
  trimmed = std::regex_replace(trimmed, std::regex("^\\s+|\\s+$"), "");

  if (trimmed.empty())
    return total_inches;

  // Handle inches part, which could be a mixed number like "110 1/8"
  size_t space_pos = trimmed.find_last_of(" \t");
  if (space_pos != std::string::npos) {
    std::string part1 = trimmed.substr(0, space_pos);
    std::string part2 = trimmed.substr(space_pos + 1);
    // Check if the second part looks like a fraction
    if (part2.find('/') != std::string::npos) {
      total_inches += parseFraction(part1);
      total_inches += parseFraction(part2);
    } else {
      // Not a "number fraction" format, so parse the whole string
      total_inches += parseFraction(trimmed);
    }
  } else {
    // No space, so it's a single number or fraction
    total_inches += parseFraction(trimmed);
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

/**
 * @brief Gets a line of input from the user, with a prompt and default value.
 * @param prompt The prompt to display to the user.
 * @param defaultValue The value to return if the user enters nothing.
 * @return The user's input or the default value.
 */
std::string getInput(const std::string &prompt,
                     const std::string &defaultValue) {
  std::cout << prompt << ": ";
  std::string line;
  std::getline(std::cin, line);

  // Trim whitespace from user input
  line.erase(0, line.find_first_not_of(" \t\n\r"));
  line.erase(line.find_last_not_of(" \t\n\r") + 1);

  if (line.empty()) {
    return defaultValue;
  }
  return line;
}
