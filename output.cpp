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
int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

/**
 * @brief Converts a decimal value into a simplified fraction string.
 *
 * @param value The decimal to convert (e.g., 0.125).
 * @return A string representing the fraction (e.g., "1/8").
 */
std::string toFraction(double value) {
  if (value == 0)
    return "0";

  const double tolerance = 1e-6;
  // Common denominators for imperial measurements up to 1/64
  const int denominators[] = {2, 4, 8, 16, 32, 64};
  int best_n = 0;
  int best_d = 1;

  for (int d : denominators) {
    int n = static_cast<int>(round(value * d));
    if (std::abs(value - static_cast<double>(n) / d) < tolerance) {
      best_n = n;
      best_d = d;
      break; // Found a precise enough fraction
    }
  }

  // If no exact fraction found, we might keep the best approximation
  // or just return decimal. For kerf, it's usually exact.

  // Simplify the fraction
  if (best_n > 0) {
    int common = gcd(best_n, best_d);
    best_n /= common;
    best_d /= common;
  }

  if (best_d == 1) {
    return std::to_string(best_n);
  }
  return std::to_string(best_n) + "/" + std::to_string(best_d);
}

// Helper function to group identical sticks into patterns for cleaner output
std::vector<Pattern> groupPatterns(const std::vector<Stick> &sticks) {
  std::map<std::string, Pattern> patternMap;

  for (const auto &stick : sticks) {
    std::vector<int> lengths;
    for (const auto &cut : stick.cuts) {
      lengths.push_back(cut.length);
    }
    std::sort(lengths.begin(), lengths.end(), std::greater<int>());

    std::stringstream key_ss;
    for (size_t i = 0; i < lengths.size(); i++) {
      if (i > 0)
        key_ss << ",";
      key_ss << lengths[i];
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

void printResults(const std::string &tubing, int stockLen, double kerf,
                  const std::vector<Cut> &cuts, const Solution &solution) {
  if (solution.num_sticks == 0) {
    std::cout << "\nNo solution found. Check input values.\n";
    return;
  }

  int totalStock = solution.num_sticks * stockLen;
  double efficiency = 0.0;
  if (totalStock > 0) {
    efficiency = static_cast<double>(totalStock - solution.total_waste) /
                 totalStock * 100.0;
  }

  std::cout << "\n--- Cut Optimization Summary ---\n";
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

  // Display the required cuts summary
  std::map<int, int> cut_counts;
  for (const auto &cut : cuts) {
    cut_counts[cut.length]++;
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

    std::map<int, int> cutCounts;
    for (const auto &cut : p.cuts) {
      cutCounts[cut.length]++;
    }

    for (auto it = cutCounts.rbegin(); it != cutCounts.rend(); ++it) {
      std::cout << "    - " << it->second << " × " << prettyLen(it->first)
                << "\n";
    }
  }
}

void generateHTML(const std::string &filename, const std::string &tubing,
                  int stockLen, double kerf, const std::vector<Cut> &cuts,
                  const Solution &solution) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not create HTML file: " << filename << std::endl;
    return;
  }

  // Get current date
  time_t now = time(0);
  tm *ltm = localtime(&now);
  std::stringstream dateStr;
  dateStr << 1900 + ltm->tm_year << "-" << std::setfill('0') << std::setw(2)
          << 1 + ltm->tm_mon << "-" << std::setw(2) << ltm->tm_mday;

  int totalStock = solution.num_sticks * stockLen;
  double efficiency =
      (totalStock > 0)
          ? static_cast<double>(totalStock - solution.total_waste) /
                totalStock * 100.0
          : 0.0;

  auto patterns = groupPatterns(solution.sticks);

  // --- Start of HTML Content ---
  file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>Cut Plan: )"
       << tubing << R"(</title>
    <style>
        :root {
            --primary: #0A3D62; --accent: #3C6382; --light: #EAF0F4;
            --gray: #F0F0F0; --border: #BDBDBD;
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
        th, td { padding: 10px 8px; border: 1px solid var(--border); vertical-align: middle; }
        th { background: var(--gray); text-align: left; font-weight: 600; }
        .summary-table td:first-child { font-weight: 600; width: 180px; }
        .stock-bar { display: flex; height: 35px; background: #f0f0f0; border: 1px solid #ccc; border-radius: 4px; overflow: hidden; margin: 4px 0; }
        .cut-piece, .waste-piece { display: flex; align-items: center; justify-content: center; color: white; font-size: 0.8rem; font-weight: bold; text-shadow: 1px 1px 1px rgba(0,0,0,0.5); border-right: 2px solid #fff; }
        .cut-piece:last-of-type { border-right: none; }
        .waste-piece { background: repeating-linear-gradient(45deg, #e0e0e0, #e0e0e0 10px, #d0d0d0 10px, #d0d0d0 20px); color: #555; font-weight: normal; text-shadow: none; }
        .cut-piece.c1 { background-color: var(--cut1); } .cut-piece.c2 { background-color: var(--cut2); }
        .cut-piece.c3 { background-color: var(--cut3); } .cut-piece.c4 { background-color: var(--cut4); }
        .cut-piece.c5 { background-color: var(--cut5); } .cut-piece.c6 { background-color: var(--cut6); }

        @media print {
            body { max-width: 100%; padding: 0; -webkit-print-color-adjust: exact; print-color-adjust: exact; }
            .header { page-break-after: avoid; }
            .grid { grid-template-columns: 1fr 1fr; } /* Keep grid on print */
            .no-print { display: none; }
            h1, h2 { page-break-after: avoid; }
            table, .stock-bar { page-break-inside: avoid; }
            tr { page-break-inside: avoid; }
            .cut-piece, .waste-piece { -webkit-print-color-adjust: exact; print-color-adjust: exact; }
        }
    </style>
</head>
<body>
<div class="header">
    <h1>Cut Plan</h1>
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
            <tr><th>Length</th><th>Quantity</th></tr>)";

  // ** NEW: Display the required cuts summary **
  std::map<int, int> cut_counts;
  for (const auto &cut : cuts) {
    cut_counts[cut.length]++;
  }
  for (auto it = cut_counts.rbegin(); it != cut_counts.rend(); ++it) {
    file << "\n            <tr><td>" << prettyLen(it->first) << "</td><td>"
         << it->second << "</td></tr>";
  }

  file << R"(
        </table>
    </div>
</div>

<h2>Cut Patterns</h2>
<table>
    <tr><th style="width:8%;">Qty</th><th>Visual Layout per Stick</th><th style="width:12%;">Used</th><th style="width:12%;">Waste</th></tr>)";

  // Map unique cut lengths to a color class for consistent coloring
  std::map<int, std::string> colorMap;
  int colorIndex = 1;

  for (const auto &p : patterns) {
    file << "\n    <tr>\n        <td>" << p.count << "</td>\n        <td>";
    file << "\n            <div class=\"stock-bar\" title=\"Used: "
         << prettyLen(p.used_len) << " | Waste: " << prettyLen(p.waste_len)
         << "\">";

    // Create a colored div for each cut piece
    for (const auto &cut : p.cuts) {
      if (colorMap.find(cut.length) == colorMap.end()) {
        colorMap[cut.length] = "c" + std::to_string(colorIndex++);
        if (colorIndex > 6)
          colorIndex = 1; // Cycle through colors
      }
      double width_percent =
          (static_cast<double>(cut.length) / stockLen) * 100.0;
      file << "\n                <div class=\"cut-piece "
           << colorMap[cut.length] << "\" style=\"width: " << std::fixed
           << std::setprecision(3) << width_percent << "%;\" title=\""
           << prettyLen(cut.length) << "\">" << prettyLen(cut.length)
           << "</div>";
    }

    // Create a striped div for the waste
    if (p.waste_len > 0) {
      double waste_percent =
          (static_cast<double>(p.waste_len) / stockLen) * 100.0;
      file << "\n                <div class=\"waste-piece\" style=\"width: "
           << std::fixed << std::setprecision(3) << waste_percent
           << "%;\" title=\"Waste: " << prettyLen(p.waste_len) << "\"></div>";
    }

    file << "\n            </div>\n        </td>";
    file << "\n        <td>" << prettyLen(p.used_len) << "</td>";
    file << "\n        <td>" << prettyLen(p.waste_len) << "</td>\n    </tr>";
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
  std::string cmd = "xdg-open \"" + filename + "\" 2>/dev/null";
#endif
  int result = system(cmd.c_str());
  if (result != 0) {
    std::cout << "Could not open file automatically. Please open '" << filename
              << "' manually.\n";
  }
}
