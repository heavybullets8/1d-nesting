#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "algorithm.h"
#include "output.h"
#include "parse.h"
#include "types.h"

// Version information
const char *VERSION = "1.0.0";
const char *BUILD_DATE = __DATE__;

// Simple test functions
void runTests() {
  std::cout << "Running tests...\n";

  // Test parseAdvancedLength
  struct TestCase {
    std::string input;
    int expected;
  };

  TestCase tests[] = {{"24'", 288},      {"288", 288},     {"20' 6\"", 246},
                      {"7'6 1/2\"", 91}, {"180 1/2", 181}, {"8'4\"", 100},
                      {"bad", 0}};

  bool allPassed = true;

  for (const auto &test : tests) {
    int result = parseAdvancedLength(test.input);
    if (result != test.expected) {
      std::cout << "FAIL: parseAdvancedLength(\"" << test.input
                << "\") = " << result << ", expected " << test.expected << "\n";
      allPassed = false;
    }
  }

  // Test parseFraction
  struct FractionTest {
    std::string input;
    double expected;
  };

  FractionTest fracTests[] = {{"1/2", 0.5},
                              {"0.125", 0.125},
                              {"3/16", 0.1875},
                              {" 3 / 6 ", 0.5},
                              {"junk", 0}};

  for (const auto &test : fracTests) {
    double result = parseFraction(test.input);
    if (std::abs(result - test.expected) > 0.0001) {
      std::cout << "FAIL: parseFraction(\"" << test.input << "\") = " << result
                << ", expected " << test.expected << "\n";
      allPassed = false;
    }
  }

  if (allPassed) {
    std::cout << "All tests passed!\n";
  } else {
    std::cout << "Some tests failed.\n";
  }
}

int main(int argc, char *argv[]) {
  // Check for test flag
  if (argc > 1 && strcmp(argv[1], "--test") == 0) {
    runTests();
    return 0;
  }

  std::cout << "--- Tube-Designer " << VERSION << " (C++ Edition) ---\n";
  std::cout << "Built: " << BUILD_DATE << "\n\n";

  // 1. Get tubing description
  std::string tubing = getInput("Tubing type (e.g. 2x2)", "2x2");

  // 2. Get stock length
  std::string stockStr = getInput("Stock length (e.g. 24' or 288)", "24'");
  int stockIn = parseAdvancedLength(stockStr);
  if (stockIn <= 0) {
    std::cerr << "Error: Stock length must be a positive number.\n";
    return 1;
  }
  std::cout << "  ✓ Using " << prettyLen(stockIn) << " stock\n";

  // 3. Get kerf size
  std::string kerfStr =
      getInput("Kerf/blade thickness (e.g. 1/8 or 0.125)", "1/8");
  double kerfIn = parseFraction(kerfStr);
  if (kerfIn <= 0) {
    kerfIn = 0.125; // Default to 1/8"
    std::cout << "  ✓ Using default kerf: " << kerfIn << "\"\n";
  } else {
    std::cout << "  ✓ Using " << kerfIn << "\" kerf\n";
  }

  // 4. Get cut list
  std::cout
      << "\nEnter cuts as 'length quantity' (e.g., '90 25' or '7'6 50').\n";
  std::cout << "Press Enter on a blank line to finish.\n";

  std::vector<Cut> cuts;
  int cutID = 1;

  while (true) {
    std::cout << "→ ";
    std::string line;
    std::getline(std::cin, line);

    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    if (line.empty()) {
      break;
    }

    // Find the last space to separate length and quantity
    size_t lastSpace = line.find_last_of(' ');
    if (lastSpace == std::string::npos) {
      std::cout << "  ✖ Invalid format. Please use 'length quantity'.\n";
      continue;
    }

    std::string lengthStr = line.substr(0, lastSpace);
    std::string qtyStr = line.substr(lastSpace + 1);

    int qty;
    try {
      qty = std::stoi(qtyStr);
    } catch (...) {
      std::cout << "  ✖ Quantity must be a positive number.\n";
      continue;
    }

    if (qty <= 0) {
      std::cout << "  ✖ Quantity must be a positive number.\n";
      continue;
    }

    int length = parseAdvancedLength(lengthStr);
    if (length <= 0) {
      std::cout << "  ✖ Could not parse length.\n";
      continue;
    }

    if (length > stockIn) {
      std::cout << "  ✖ Cut of " << prettyLen(length)
                << " is longer than stock of " << prettyLen(stockIn) << ".\n";
      continue;
    }

    // Add cuts
    for (int i = 0; i < qty; i++) {
      cuts.push_back(Cut(length, cutID++));
    }

    std::cout << "  ✓ Added " << qty << " × " << prettyLen(length) << "\n";
  }

  if (cuts.empty()) {
    std::cout << "No cuts entered. Exiting.\n";
    return 0;
  }

  // 5. Optimize
  std::cout << "\nOptimizing " << cuts.size() << " total cuts...\n";

  auto startTime = std::chrono::high_resolution_clock::now();
  Solution solution = optimizeCutting(cuts, stockIn, kerfIn);
  auto endTime = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);
  std::cout << "Optimization finished in " << duration.count() / 1000.0
            << " seconds.\n";

  // 6. Print results
  printResults(tubing, stockIn, kerfIn, cuts, solution);

  // 7. Generate HTML
  std::string htmlFile = "cut_plan.html";
  generateHTML(htmlFile, tubing, stockIn, kerfIn, cuts, solution);
  openFile(htmlFile);

  return 0;
}
