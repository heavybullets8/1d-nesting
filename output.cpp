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

std::vector<Pattern> groupPatterns(const std::vector<Stick> &sticks) {
  std::map<std::string, Pattern *> patternMap;

  for (const auto &stick : sticks) {
    // Create a key based on sorted cut lengths
    std::vector<int> lengths;
    for (const auto &cut : stick.cuts) {
      lengths.push_back(cut.length);
    }
    std::sort(lengths.begin(), lengths.end(), std::greater<int>());

    std::stringstream key;
    for (size_t i = 0; i < lengths.size(); i++) {
      if (i > 0)
        key << "-";
      key << lengths[i];
    }

    auto keyStr = key.str();
    if (patternMap.find(keyStr) != patternMap.end()) {
      patternMap[keyStr]->count++;
    } else {
      Pattern *p = new Pattern();
      p->cuts = stick.cuts;
      p->count = 1;
      p->used_len = stick.used_len;
      p->waste_len = stick.waste_len;
      patternMap[keyStr] = p;
    }
  }

  std::vector<Pattern> patterns;
  for (const auto &[key, p] : patternMap) {
    patterns.push_back(*p);
    delete p;
  }

  // Sort by count (descending), then by used length
  std::sort(patterns.begin(), patterns.end(),
            [](const Pattern &a, const Pattern &b) {
              if (a.count == b.count) {
                return a.used_len < b.used_len;
              }
              return a.count > b.count;
            });

  return patterns;
}

void printResults(const std::string &tubing, int stockLen, double kerf,
                  const std::vector<Cut> &cuts, const Solution &solution) {
  int totalStock = solution.num_sticks * stockLen;
  double efficiency = 0.0;
  if (totalStock > 0) {
    efficiency = static_cast<double>(totalStock - solution.total_waste) /
                 totalStock * 100.0;
  }

  std::cout << "\n--- Cut Optimization Summary ---\n";
  std::cout << "Material:      " << tubing << " @ " << prettyLen(stockLen)
            << "\n";
  std::cout << "Sticks Needed: " << solution.num_sticks << "\n";
  std::cout << "Efficiency:    " << std::fixed << std::setprecision(1)
            << efficiency << "%\n";
  std::cout << "Total Waste:   " << prettyLen(solution.total_waste) << " (avg "
            << prettyLen(solution.total_waste / solution.num_sticks)
            << " per stick)\n";
  std::cout << "---------------------------------\n";

  auto patterns = groupPatterns(solution.sticks);
  std::cout << "\nCut Patterns (Qty | Cuts -> Waste):\n";

  for (const auto &p : patterns) {
    std::cout << "  " << std::setw(2) << p.count << " × | ";

    for (size_t i = 0; i < p.cuts.size(); i++) {
      if (i > 0)
        std::cout << ", ";
      std::cout << prettyLen(p.cuts[i].length);
    }

    std::cout << " -> " << prettyLen(p.waste_len) << " waste\n";
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

  // Start HTML output
  file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>Cut Plan</title>
    <style>
        :root {
            --primary: #05445E; --accent: #189AB4; --light: #D4F1F4;
            --gray: #ECECEC; --border: #C7C7C7;
        }
        * { box-sizing: border-box; }
        body { font-family: "Segoe UI", Helvetica, Arial, sans-serif; margin: 0 auto; max-width: 960px; padding: 24px; color: #333; background: #fff; }
        h1 { color: var(--primary); margin-top: 0; }
        h2 { color: var(--accent); border-bottom: 2px solid var(--accent); padding-bottom: 4px; }
        table { width: 100%; border-collapse: collapse; margin: 16px 0; }
        th, td { padding: 10px 8px; border: 1px solid var(--border); }
        th { background: var(--gray); text-align: left; }
        tr:nth-child(even) td { background: var(--light); }
        ul { margin: 0 0 16px 20px; }
        .tag { display: inline-block; background: var(--accent); color: #fff; padding: 2px 8px; border-radius: 4px; font-size: 0.8rem; margin-left: 6px; }
    </style>
</head>
<body>
<h1>Cut Plan</h1>
<p>
    <strong>Date:</strong> )"
       << dateStr.str() << R"(<br>
    <strong>Material:</strong> )"
       << tubing << " @ " << prettyLen(stockLen) << R"(<br>
    <strong>Kerf:</strong> )"
       << std::fixed << std::setprecision(4) << kerf << R"("<br>
    <strong>Sticks needed:</strong> )"
       << solution.num_sticks << " × " << prettyLen(stockLen) << R"(
</p>
<h2>Efficiency Summary</h2>
<ul>
    <li>Total stock used: )"
       << prettyLen(totalStock) << R"(</li>
    <li>Total waste: )"
       << prettyLen(solution.total_waste) << R"(</li>
    <li>Material efficiency: )"
       << std::fixed << std::setprecision(1) << efficiency << R"(%</li>
    <li>Average waste per stick: )"
       << prettyLen(solution.total_waste / solution.num_sticks) << R"(</li>
</ul>
<h2>Cut Patterns</h2>
<table>
    <tr><th>Qty</th><th>Cuts</th><th>Used</th><th>Waste</th></tr>)";

  // Pattern summary table
  for (const auto &p : patterns) {
    file << "\n    <tr>\n        <td>" << p.count << "</td>\n        <td>";

    for (size_t i = 0; i < p.cuts.size(); i++) {
      if (i > 0)
        file << ", ";
      file << prettyLen(p.cuts[i].length);
    }

    file << "</td>\n        <td>" << prettyLen(p.used_len)
         << "</td>\n        <td>" << prettyLen(p.waste_len)
         << "</td>\n    </tr>";
  }

  file << "\n</table>\n";

  // Detailed cut instructions for each pattern
  int patternNum = 1;
  int kerfInt = static_cast<int>(std::ceil(kerf * 1000));

  for (const auto &p : patterns) {
    file << "\n<h3>Pattern " << patternNum << "<span class=\"tag\">Qty "
         << p.count << "</span></h3>\n";
    file
        << "<table>\n    <tr><th>#</th><th>Mark At</th><th>Cut Piece</th></tr>";

    int runningLen = 0;
    for (size_t i = 0; i < p.cuts.size(); i++) {
      if (i > 0) {
        runningLen += kerfInt / 1000;
      }
      int markAt = runningLen + p.cuts[i].length;

      file << "\n    <tr><td>" << (i + 1) << "</td><td>" << prettyLen(markAt)
           << "</td><td>" << prettyLen(p.cuts[i].length) << "</td></tr>";

      runningLen = markAt;
    }

    file << "\n    <tr><td colspan=\"3\">Remaining: " << prettyLen(p.waste_len)
         << "</td></tr>\n</table>\n";

    patternNum++;
  }

  file << R"(
</body>
</html>)";

  file.close();
  std::cout << "\nDetailed cut plan saved to " << filename << std::endl;
}

void openFile(const std::string &filename) {
  // Use xdg-open for Linux
  std::string cmd = "xdg-open " + filename + " 2>/dev/null &";
  int result = system(cmd.c_str());
  if (result != 0) {
    std::cout << "Could not open file automatically. Please open " << filename
              << " manually.\n";
  }
}
