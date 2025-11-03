#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>

// Single-header libraries (will be downloaded to include/ directory)
#include "httplib.h"
#include "json.hpp"

// Project headers
#include "algorithm.h"
#include "output.h"
#include "parse.h"
#include "types.h"

using json = nlohmann::json;

httplib::Server* g_svr = nullptr;

void signalHandler(int signum) {
  if (g_svr) {
    json log;
    log["timestamp"] = std::time(nullptr);
    log["level"] = "INFO";
    log["message"] = "Shutting down server gracefully";
    std::cout << log.dump() << std::endl;
    g_svr->stop();
  }
}

class Logger {
public:
  enum Level { DEBUG, INFO, WARN, ERROR };

  static std::string levelToString(Level level) {
    switch (level) {
      case DEBUG: return "DEBUG";
      case INFO: return "INFO";
      case WARN: return "WARN";
      case ERROR: return "ERROR";
      default: return "UNKNOWN";
    }
  }

  static void log(Level level, const std::string& message) {
    json log;
    log["timestamp"] = std::time(nullptr);
    log["level"] = levelToString(level);
    log["message"] = message;
    std::cout << log.dump() << std::endl;
  }

  static void logRequest(const std::string& method, const std::string& path,
                         int status, const std::string& remoteAddr,
                         const std::string& remotePort, double duration_ms = -1) {
    if (path == "/" || path == "/api/health") {
      return;
    }

    json log;
    log["timestamp"] = std::time(nullptr);
    log["level"] = status >= 400 ? "ERROR" : "INFO";
    log["type"] = "http_request";
    log["method"] = method;
    log["path"] = path;
    log["status"] = status;
    log["remote_addr"] = remoteAddr;
    log["remote_port"] = remotePort;
    if (duration_ms >= 0) {
      log["duration_ms"] = duration_ms;
    }
    std::cout << log.dump() << std::endl;
  }
};

// Read file content
std::string readFile(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    // Try alternate path (useful for Docker deployments)
    std::string altPath = "/app/" + path;
    file.open(altPath);
    if (!file.is_open()) {
      return "";
    }
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Convert Solution to JSON for API response
json solutionToJson(const Solution& solution, double stockLen, double kerf) {
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

  for (const auto& p : patterns) {
    json pattern;
    pattern["count"] = p.count;
    pattern["used_len"] = p.used_len;
    pattern["waste_len"] = p.waste_len;

    json cutsJson = json::array();
    for (const auto& cut : p.cuts) {
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
  // Set up signal handling for graceful shutdown
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  // Check for static files
  std::string indexContent = readFile("static/index.html");
  if (indexContent.empty()) {
    Logger::log(Logger::WARN,
                "static/index.html not found in current directory");
    Logger::log(Logger::INFO,
                "Will try /app/static/index.html when requests come in");
  } else {
    Logger::log(Logger::INFO, "Static files found in current directory");
  }

  httplib::Server svr;
  g_svr = &svr; // Store global reference for signal handler

  // Set up request logging
  svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
    Logger::logRequest(req.method, req.path, res.status,
                       req.remote_addr, std::to_string(req.remote_port));
  });

  // Note: CORS headers will be set in each handler

  // Serve the main page
  svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
    std::string content = readFile("static/index.html");
    if (content.empty()) {
      Logger::log(Logger::ERROR,
                  "Failed to read static/index.html - file not found");
      res.status = 404;
      res.set_content("<h1>404 - File Not Found</h1><p>Could not find "
                      "index.html. Please check your deployment.</p>",
                      "text/html");
    } else {
      res.set_content(content, "text/html");
    }
  });

  // Serve static files
  svr.Get("/static/(.*)",
          [](const httplib::Request& req, httplib::Response& res) {
            std::string path = "static/" + req.matches[1].str();
            std::string content = readFile(path);
            if (content.empty()) {
              Logger::log(Logger::WARN, "Static file not found: " + path);
              res.status = 404;
              res.set_content("File not found", "text/plain");
            } else {
              // Set appropriate content type based on extension
              std::string ext = path.substr(path.find_last_of(".") + 1);
              std::string content_type = "text/plain";
              if (ext == "html")
                content_type = "text/html";
              else if (ext == "css")
                content_type = "text/css";
              else if (ext == "js")
                content_type = "application/javascript";
              else if (ext == "json")
                content_type = "application/json";

              res.set_content(content, content_type);
            }
          });

  // Health check endpoint
  svr.Get("/api/health",
          [](const httplib::Request& req, httplib::Response& res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
          });

  // Handle OPTIONS requests for CORS
  svr.Options(
      "/api/optimize", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
      });

  // Main optimization endpoint
  svr.Post("/api/optimize", [](const httplib::Request& req,
                               httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Content-Type", "application/json");

    try {
      Logger::log(Logger::DEBUG, "Parsing optimization request body");
      auto body = json::parse(req.body);

      // Extract parameters
      std::string jobName = body.value("jobName", "Cut Plan");
      std::string materialType =
          body.value("materialType", "Standard Material");
      std::string stockLengthStr = body["stockLength"];
      std::string kerfStr = body["kerf"];
      auto cutsArray = body["cuts"];

      // Parse stock length
      double stockLen = parseAdvancedLength(stockLengthStr);
      if (stockLen <= 0) {
        Logger::log(Logger::WARN, "Invalid stock length: " + stockLengthStr);
        res.status = 400;
        res.set_content("{\"error\":\"Invalid stock length\"}",
                        "application/json");
        return;
      }

      // Parse kerf
      double kerf = parseFraction(kerfStr);
      if (kerf <= 0) {
        kerf = 0.125; // Default to 1/8"
        Logger::log(Logger::INFO, "Using default kerf: 1/8\"");
      }

      // Parse cuts
      std::vector<Cut> cuts;
      int cutID = 1;
      int totalCuts = 0;

      for (const auto& cutItem : cutsArray) {
        double length =
            parseAdvancedLength(cutItem["length"].get<std::string>());
        int quantity = cutItem["quantity"].get<int>();

        if (length <= 0 || quantity <= 0) {
          Logger::log(Logger::WARN,
                      "Skipping invalid cut: length=" + std::to_string(length) +
                          ", qty=" + std::to_string(quantity));
          continue;
        }

        if (length > stockLen) {
          Logger::log(Logger::ERROR,
                      "Cut length exceeds stock: " + std::to_string(length) +
                          " > " + std::to_string(stockLen));
          res.status = 400;
          res.set_content("{\"error\":\"Cut length exceeds stock length\"}",
                          "application/json");
          return;
        }

        for (int i = 0; i < quantity; i++) {
          cuts.push_back(Cut(length, cutID++));
          totalCuts++;
        }
      }

      if (cuts.empty()) {
        Logger::log(Logger::WARN, "No valid cuts provided");
        res.status = 400;
        res.set_content("{\"error\":\"No valid cuts provided\"}",
                        "application/json");
        return;
      }

      // Log optimization parameters
      std::stringstream logMsg;
      logMsg << "Starting optimization - Job: " << jobName
             << ", Stock: " << stockLen << "\", Kerf: " << kerf
             << "\", Total cuts: " << totalCuts;
      Logger::log(Logger::INFO, logMsg.str());

      // Run optimization
      auto startTime = std::chrono::high_resolution_clock::now();
      Solution solution = optimizeCutting(cuts, stockLen, kerf);
      auto endTime = std::chrono::high_resolution_clock::now();

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          endTime - startTime);

      if (solution.num_sticks == 0) {
        Logger::log(Logger::ERROR, "Optimization failed - no solution found");
        res.status = 500;
        res.set_content("{\"error\":\"No solution found\"}",
                        "application/json");
        return;
      }

      // Log results
      logMsg.str("");
      logMsg << "Optimization complete - Sticks: " << solution.num_sticks
             << ", Waste: " << solution.total_waste
             << "\", Time: " << duration.count() << "ms";
      Logger::log(Logger::INFO, logMsg.str());

      // Prepare response
      json response;
      response["jobName"] = jobName;
      response["materialType"] = materialType;
      response["stockLength"] = stockLen;
      response["stockLengthPretty"] = prettyLen(stockLen);
      response["kerf"] = kerf;
      response["kerfPretty"] = toFraction(kerf);
      response["solution"] = solutionToJson(solution, stockLen, kerf);
      response["optimizationTime"] = duration.count() / 1000.0;

      // Group cuts by length for summary
      std::map<double, int> cutCounts;
      for (const auto& cut : cuts) {
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

    } catch (const json::parse_error& e) {
      Logger::log(Logger::ERROR, "JSON parse error: " + std::string(e.what()));
      json error;
      error["error"] = "Invalid JSON format";
      res.status = 400;
      res.set_content(error.dump(), "application/json");
    } catch (const std::exception& e) {
      Logger::log(Logger::ERROR, "Server error: " + std::string(e.what()));
      json error;
      error["error"] = std::string("Server error: ") + e.what();
      res.status = 500;
      res.set_content(error.dump(), "application/json");
    }
  });

  // Handle 404s
  svr.set_error_handler(
      [](const httplib::Request& req, httplib::Response& res) {
        if (res.status == 404) {
          json error;
          error["error"] = "Not found";
          error["path"] = req.path;
          res.set_content(error.dump(), "application/json");
          Logger::log(Logger::WARN, "404 Not Found: " + req.path);
        }
      });

  Logger::log(Logger::INFO, "==========================================");
  Logger::log(Logger::INFO, "    1D Nesting Software Server v1.0");
  Logger::log(Logger::INFO, "==========================================");
  Logger::log(Logger::INFO, "Starting server on http://0.0.0.0:8080");
  Logger::log(Logger::INFO, "Available endpoints:");
  Logger::log(Logger::INFO, "  GET  /              - Web interface");
  Logger::log(Logger::INFO, "  GET  /api/health    - Health check");
  Logger::log(Logger::INFO, "  POST /api/optimize  - Run optimization");
  Logger::log(Logger::INFO, "==========================================");
  Logger::log(Logger::INFO, "Press Ctrl+C to stop");

  if (!svr.listen("0.0.0.0", 8080)) {
    Logger::log(Logger::ERROR, "Failed to start server - port may be in use");
    return 1;
  }

  Logger::log(Logger::INFO, "Server stopped");
  return 0;
}
