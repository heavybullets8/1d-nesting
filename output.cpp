#include "output.h"
#include "parse.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

// Helper to find the greatest common divisor for fraction simplification
// Made static to prevent linker errors from multiple definitions.
static int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

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

// Helper function to group identical sticks into patterns for cleaner output
std::vector<Pattern> groupPatterns(const std::vector<Stick> &sticks) {
  std::map<std::string, Pattern> patternMap;

  for (const auto &stick : sticks) {
    std::vector<double> lengths;
    for (const auto &cut : stick.cuts) {
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
                [](const Cut &a, const Cut &b) { return a.length > b.length; });
      p.count = 1;
      p.used_len = stick.used_len;
      p.waste_len = stick.waste_len;
      patternMap[key] = p;
    }
  }

  std::vector<Pattern> patterns;
  for (const auto &[key, p] : patternMap) {
    patterns.push_back(p);
  }

  std::sort(patterns.begin(), patterns.end(),
            [](const Pattern &a, const Pattern &b) {
              if (a.count != b.count) {
                return a.count > b.count;
              }
              return a.used_len > b.used_len;
            });

  return patterns;
}

void printResults(const std::string &jobName, const std::string &tubing,
                  double stockLen, double kerf, const std::vector<Cut> &cuts,
                  const Solution &solution) {
  if (solution.num_sticks == 0) {
    std::cout << "\nNo solution found. Check input values.\n";
    return;
  }

  double totalStock = solution.num_sticks * stockLen;
  double efficiency = 0.0;
  if (totalStock > 0) {
    efficiency = (totalStock - solution.total_waste) / totalStock * 100.0;
  }

  std::cout << "\n--- " << jobName << " Summary ---\n";
  std::cout << "Material:      " << tubing << " @ " << prettyLen(stockLen)
            << "\n";
  std::cout << "Kerf:          " << toFraction(kerf) << "\"\n";
  std::cout << "Sticks Needed: " << solution.num_sticks << "\n";
  std::cout << "Efficiency:    " << std::fixed << std::setprecision(1)
            << efficiency << "%\n";
  std::cout << "Total Waste:   " << prettyLen(solution.total_waste) << " (avg "
            << prettyLen(solution.total_waste / solution.num_sticks)
            << " per stick)\n";
  std::cout << "---------------------------------\n";

  // Display the required cuts summary, grouping by length
  std::map<double, int> cut_counts;
  for (const auto &cut : cuts) {
    // Round to a certain precision to group similar floats
    double key = std::round(cut.length * 10000.0) / 10000.0;
    cut_counts[key]++;
  }
  std::cout << "\nRequired Cuts (" << cuts.size() << " total pieces):\n";
  for (auto it = cut_counts.rbegin(); it != cut_counts.rend(); ++it) {
    std::cout << "  - " << std::setw(3) << it->second << " × "
              << prettyLen(it->first) << "\n";
  }

  auto patterns = groupPatterns(solution.sticks);
  std::cout << "\nCut Patterns:\n";

  for (const auto &p : patterns) {
    std::cout << "  " << p.count
              << " × Sticks (Waste: " << prettyLen(p.waste_len) << ")\n";

    std::map<double, int> cutCounts;
    for (const auto &cut : p.cuts) {
      double key = std::round(cut.length * 10000.0) / 10000.0;
      cutCounts[key]++;
    }

    for (auto it = cutCounts.rbegin(); it != cutCounts.rend(); ++it) {
      std::cout << "    - " << it->second << " × " << prettyLen(it->first)
                << "\n";
    }
  }
}

void generateHTML(const std::string &filename, const std::string &jobName,
                  const std::string &tubing, double stockLen, double kerf,
                  const std::vector<Cut> &cuts, const Solution &solution) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not create HTML file: " << filename << std::endl;
    return;
  }

  time_t now = time(0);
  tm *ltm = localtime(&now);
  std::stringstream dateStr;
  dateStr << 1900 + ltm->tm_year << "-" << std::setfill('0') << std::setw(2)
          << 1 + ltm->tm_mon << "-" << std::setw(2) << ltm->tm_mday;

  double totalStock = solution.num_sticks * stockLen;
  double efficiency = (totalStock > 0) ? (totalStock - solution.total_waste) /
                                             totalStock * 100.0
                                       : 0.0;

  auto patterns = groupPatterns(solution.sticks);

  // --- Start of HTML Content ---
  file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>)"
       << jobName << ": " << tubing << R"(</title>
    <style>
        :root {
            --primary: #0A3D62; --accent: #3C6382; --light: #EAF0F4;
            --gray: #F0F0F0; --border: #AAAAAA; --waste-bg: #e0e0e0;
            --cut1: #1f77b4; --cut2: #ff7f0e; --cut3: #2ca02c;
            --cut4: #d62728; --cut5: #9467bd; --cut6: #8c564b;
        }
        * { box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; margin: 0 auto; max-width: 1000px; padding: 20px; color: #333; background: #fff; }
        .header { display: flex; justify-content: space-between; align-items: flex-start; border-bottom: 3px solid var(--primary); padding-bottom: 10px; margin-bottom: 20px; }
        h1 { color: var(--primary); margin: 0; font-size: 2em; }
        .header p { margin: 0; text-align: right; color: #555; }
        h2 { color: var(--accent); border-bottom: 2px solid var(--accent); padding-bottom: 5px; margin-top: 30px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { padding: 10px 8px; border: 1px solid var(--border); vertical-align: middle; text-align: left; }
        th { background: var(--gray); font-weight: 600; }
        td:last-child, th:last-child { text-align: right; }
        .summary-table td:first-child { font-weight: 600; width: 180px; }
        .summary-table td:last-child { text-align: left; }
        .stock-bar { display: flex; height: 35px; background: var(--gray); border: 1px solid #ccc; border-radius: 4px; overflow: hidden; margin: 4px 0; }
        .cut-piece, .waste-piece { display: flex; align-items: center; justify-content: center; color: white; font-size: 0.8rem; font-weight: bold; text-shadow: 1px 1px 1px rgba(0,0,0,0.5); border-right: 2px solid #fff; white-space: nowrap; overflow: hidden; }
        .cut-piece:last-of-type { border-right: none; }
        .waste-piece { background: repeating-linear-gradient(45deg, var(--waste-bg), var(--waste-bg) 10px, #d0d0d0 10px, #d0d0d0 20px); color: #555; font-weight: normal; text-shadow: none; }
        .cut-piece.c1 { background-color: var(--cut1); } .cut-piece.c2 { background-color: var(--cut2); }
        .cut-piece.c3 { background-color: var(--cut3); } .cut-piece.c4 { background-color: var(--cut4); }
        .cut-piece.c5 { background-color: var(--cut5); } .cut-piece.c6 { background-color: var(--cut6); }
        @media print {
            body { max-width: 100%; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
            .no-print { display: none; }
            h1, h2 { page-break-after: avoid; }
            table { page-break-inside: avoid; }
        }
    </style>
</head>
<body>
<div class="header">
    <h1>)"
       << jobName << R"(</h1>
    <p><strong>Material:</strong> )"
       << tubing << R"(<br><strong>Date:</strong> )" << dateStr.str() << R"(</p>
</div>
<div class="grid">
    <div>
        <h2>Project Summary</h2>
        <table class="summary-table">
            <tr><td>Stock Length</td><td>)"
       << prettyLen(stockLen) << R"(</td></tr>
            <tr><td>Kerf / Blade</td><td>)"
       << toFraction(kerf) << R"("</td></tr>
            <tr><td>Sticks Needed</td><td>)"
       << solution.num_sticks << R"(</td></tr>
            <tr><td>Total Waste</td><td>)"
       << prettyLen(solution.total_waste) << R"(</td></tr>
            <tr><td>Efficiency</td><td>)"
       << std::fixed << std::setprecision(1) << efficiency << R"(%</td></tr>
        </table>
    </div>
    <div>
        <h2>Required Cuts</h2>
        <table>
            <tr><th>Quantity</th><th>Length</th></tr>)";

  std::map<double, int> cut_counts;
  for (const auto &cut : cuts) {
    double key = std::round(cut.length * 10000.0) / 10000.0;
    cut_counts[key]++;
  }
  for (auto it = cut_counts.rbegin(); it != cut_counts.rend(); ++it) {
    file << "\n            <tr><td>" << it->second << "</td><td>"
         << prettyLen(it->first) << "</td></tr>";
  }

  file << R"(
        </table>
    </div>
</div>
<h2>Cut Patterns</h2>
<table>
    <tr><th style="width:8%;">Qty</th><th>Visual Layout per Stick</th><th style="width:12%;">Used</th><th style="width:12%;">Waste</th></tr>)";

  std::map<double, std::string> colorMap;
  int colorIndex = 1;

  for (const auto &p : patterns) {
    file << "\n    <tr>\n        <td style='text-align:right;'>" << p.count
         << "</td>\n        <td>";
    file << "\n            <div class=\"stock-bar\" title=\"Used: "
         << prettyLen(p.used_len) << " | Waste: " << prettyLen(p.waste_len)
         << "\">";

    for (const auto &cut : p.cuts) {
      double key = std::round(cut.length * 10000.0) / 10000.0;
      if (colorMap.find(key) == colorMap.end()) {
        colorMap[key] = "c" + std::to_string(colorIndex++);
        if (colorIndex > 6)
          colorIndex = 1;
      }
      double width_percent = (cut.length / stockLen) * 100.0;
      file << "\n                <div class=\"cut-piece " << colorMap[key]
           << "\" style=\"width: " << std::fixed << std::setprecision(3)
           << width_percent << "%;\" title=\"" << prettyLen(cut.length) << "\">"
           << prettyLen(cut.length) << "</div>";
    }

    if (p.waste_len > 1.0 / 64.0) {
      double waste_percent = (p.waste_len / stockLen) * 100.0;
      file << "\n                <div class=\"waste-piece\" style=\"width: "
           << std::fixed << std::setprecision(3) << waste_percent
           << "%;\" title=\"Waste: " << prettyLen(p.waste_len) << "\"></div>";
    }

    file << "\n            </div>\n        </td>";
    file << "\n        <td style='text-align:right;'>" << prettyLen(p.used_len)
         << "</td>";
    file << "\n        <td style='text-align:right;'>" << prettyLen(p.waste_len)
         << "</td>\n    </tr>";
  }

  file << "\n</table>\n</body>\n</html>";
  file.close();
  std::cout << "\nVisual cut plan saved to " << filename << std::endl;
}

void openFile(const std::string &filename) {
#if defined(_WIN32)
  std::string cmd = "start \"\" \"" + filename + "\"";
#elif defined(__APPLE__)
  std::string cmd = "open " + filename;
#else // Linux
  std::string cmd = "xdg-open \"" + filename + "\"";
#endif
  int result = system(cmd.c_str());
  if (result != 0) {
    // Suppress the "not found" message on some systems
  }
}
