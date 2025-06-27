#ifndef PARSE_H
#define PARSE_H

#include <string>

// Parse various length formats (e.g., "24'", "8'4\"", "180 1/2", "288")
double parseAdvancedLength(const std::string &s);

// Parse a fraction or decimal (e.g., "1/2", "0.125", "3/16")
double parseFraction(const std::string &s);

// Format inches as feet and inches (e.g., 100.5 -> "8' 4 1/2\"")
std::string prettyLen(double inches);

// Get user input with a default value
std::string getInput(const std::string &prompt,
                     const std::string &defaultValue = "");

#endif // PARSE_H
