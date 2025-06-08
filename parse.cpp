#include "parse.h"
#include <cmath>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

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
    // If not a fraction, try to parse as a simple number, ensuring the whole
    // string is used.
    try {
      size_t pos;
      double val = std::stod(trimmed, &pos);
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
 * @brief Parses various length formats into inches.
 *
 * This function can handle:
 * - Simple inches: "288"
 * - Feet notation: "24'"
 * - Feet and inches: "7'6\""
 * - Inches with fractions: "6 1/2"
 * - Mixed numbers: "180 1/2"
 *
 * @param s The input string.
 * @return The total length in inches, rounded to the nearest whole number.
 */
int parseAdvancedLength(const std::string &s) {
  std::string trimmed = std::regex_replace(s, std::regex("^\\s+|\\s+$"), "");
  if (trimmed.empty()) {
    return 0;
  }

  double total_inches = 0;
  std::string feet_str, inches_str;

  // Check for the feet marker (') and split the string
  size_t feet_pos = trimmed.find('\'');
  if (feet_pos != std::string::npos) {
    feet_str = trimmed.substr(0, feet_pos);
    inches_str = trimmed.substr(feet_pos + 1);
  } else {
    // No feet marker, the whole string is treated as inches
    inches_str = trimmed;
  }

  // Parse the feet part if it exists
  if (!feet_str.empty()) {
    total_inches += parseFraction(feet_str) * 12.0;
  }

  // Parse the inches part if it exists
  if (!inches_str.empty()) {
    // Remove trailing double-quote if present
    if (!inches_str.empty() && inches_str.back() == '"') {
      inches_str.pop_back();
    }
    // Trim whitespace again
    inches_str = std::regex_replace(inches_str, std::regex("^\\s+|\\s+$"), "");

    if (!inches_str.empty()) {
      // Check for a mixed number format like "6 1/2"
      std::regex re(R"((\d+)\s+(\d+\s*\/\s*\d+))");
      std::smatch matches;
      if (std::regex_match(inches_str, matches, re)) {
        total_inches += std::stod(matches[1].str());
        total_inches += parseFraction(matches[2].str());
      } else {
        // Otherwise, the remainder is just inches, a fraction, or a decimal
        total_inches += parseFraction(inches_str);
      }
    }
  }

  return static_cast<int>(round(total_inches));
}

/**
 * @brief Formats a total number of inches into a human-readable string.
 * e.g., 100 becomes "8'4\""
 * e.g., 288 becomes "24'"
 * @param total_inches The length in inches.
 * @return A formatted string.
 */
std::string prettyLen(int total_inches) {
  if (total_inches == 0)
    return "0\"";

  std::stringstream ss;
  int feet = total_inches / 12;
  int inches = total_inches % 12;

  // Account for negative lengths (waste)
  if (total_inches < 0) {
    ss << "-";
    feet = std::abs(feet);
    inches = std::abs(inches);
  }

  if (feet > 0) {
    ss << feet << "'";
    if (inches > 0) {
      // No space needed between feet and inches for this format
      ss << inches << "\"";
    }
  } else {
    ss << inches << "\"";
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
