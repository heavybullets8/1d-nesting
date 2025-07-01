#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

// Single header HTTP library - download from
// https://github.com/yhirose/cpp-httplib
#include "httplib.h"

// Single header JSON library - download from https://github.com/nlohmann/json
#include "json.hpp"

#include "algorithm.h"
#include "output.h"
#include "parse.h"
#include "types.h"

using json = nlohmann::json;

// Read file content
std::string readFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Convert Solution to JSON for API response
json solutionToJson(const Solution &solution, double stockLen, double kerf) {
  json result;
  result["num_sticks"] = solution.num_sticks;
  result["total_waste"] = solution.total_waste;

  double totalStock = solution.num_sticks * stockLen;
  result["efficiency"] =
      totalStock > 0 ? (totalStock - solution.total_waste) / totalStock * 100.0
                     : 0.0;

  // Group patterns
  auto patterns = groupPatterns(solution.sticks);
  json patternsJson = json::array();

  for (const auto &p : patterns) {
    json pattern;
    pattern["count"] = p.count;
    pattern["used_len"] = p.used_len;
    pattern["waste_len"] = p.waste_len;

    json cutsJson = json::array();
    for (const auto &cut : p.cuts) {
      json cutJson;
      cutJson["length"] = cut.length;
      cutJson["pretty_length"] = prettyLen(cut.length);
      cutsJson.push_back(cutJson);
    }
    pattern["cuts"] = cutsJson;
    patternsJson.push_back(pattern);
  }

  result["patterns"] = patternsJson;
  return result;
}

int main() {
  httplib::Server svr;

  // Note: CORS headers will be set in each handler

  // Serve the main page
  svr.Get("/", [](const httplib::Request &req, httplib::Response &res) {
    std::string content = readFile("static/index.html");
    if (content.empty()) {
      res.status = 404;
      res.set_content("index.html not found", "text/plain");
    } else {
      res.set_content(content, "text/html");
    }
  });

  // Health check endpoint
  svr.Get("/api/health",
          [](const httplib::Request &req, httplib::Response &res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
          });

  // Handle OPTIONS requests for CORS
  svr.Options(
      "/api/optimize", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
      });

  // Main optimization endpoint
  svr.Post(
      "/api/optimize", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");

        try {
          auto body = json::parse(req.body);

          // Extract parameters
          std::string jobName = body.value("jobName", "Cut Plan");
          std::string tubing = body.value("tubing", "2x2");
          std::string stockLengthStr = body["stockLength"];
          std::string kerfStr = body["kerf"];
          auto cutsArray = body["cuts"];

          // Parse stock length
          double stockLen = parseAdvancedLength(stockLengthStr);
          if (stockLen <= 0) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid stock length\"}",
                            "application/json");
            return;
          }

          // Parse kerf
          double kerf = parseFraction(kerfStr);
          if (kerf <= 0) {
            kerf = 0.125; // Default to 1/8"
          }

          // Parse cuts
          std::vector<Cut> cuts;
          int cutID = 1;

          for (const auto &cutItem : cutsArray) {
            double length =
                parseAdvancedLength(cutItem["length"].get<std::string>());
            int quantity = cutItem["quantity"].get<int>();

            if (length <= 0 || quantity <= 0) {
              continue;
            }

            if (length > stockLen) {
              res.status = 400;
              res.set_content("{\"error\":\"Cut length exceeds stock length\"}",
                              "application/json");
              return;
            }

            for (int i = 0; i < quantity; i++) {
              cuts.push_back(Cut(length, cutID++));
            }
          }

          if (cuts.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"No valid cuts provided\"}",
                            "application/json");
            return;
          }

          // Run optimization
          auto startTime = std::chrono::high_resolution_clock::now();
          Solution solution = optimizeCutting(cuts, stockLen, kerf);
          auto endTime = std::chrono::high_resolution_clock::now();

          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              endTime - startTime);

          if (solution.num_sticks == 0) {
            res.status = 500;
            res.set_content("{\"error\":\"No solution found\"}",
                            "application/json");
            return;
          }

          // Prepare response
          json response;
          response["jobName"] = jobName;
          response["tubing"] = tubing;
          response["stockLength"] = stockLen;
          response["stockLengthPretty"] = prettyLen(stockLen);
          response["kerf"] = kerf;
          response["kerfPretty"] = toFraction(kerf);
          response["solution"] = solutionToJson(solution, stockLen, kerf);
          response["optimizationTime"] = duration.count() / 1000.0;

          // Group cuts by length for summary
          std::map<double, int> cutCounts;
          for (const auto &cut : cuts) {
            cutCounts[cut.length]++;
          }

          json cutsSum = json::array();
          for (auto it = cutCounts.rbegin(); it != cutCounts.rend(); ++it) {
            json item;
            item["length"] = it->first;
            item["lengthPretty"] = prettyLen(it->first);
            item["quantity"] = it->second;
            cutsSum.push_back(item);
          }
          response["cutsSummary"] = cutsSum;

          res.set_content(response.dump(), "application/json");

        } catch (const std::exception &e) {
          json error;
          error["error"] = std::string("Server error: ") + e.what();
          res.status = 500;
          res.set_content(error.dump(), "application/json");
        }
      });

  std::cout << "Tube Designer server starting on http://0.0.0.0:8080\n";
  std::cout << "Press Ctrl+C to stop\n";

  if (!svr.listen("0.0.0.0", 8080)) {
    std::cerr << "Failed to start server\n";
    return 1;
  }

  return 0;
}
