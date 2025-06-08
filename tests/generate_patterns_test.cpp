#include "algorithm.h"
#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

int main() {
    std::vector<int> cuts = {60, 40, 20};
    auto patterns = generatePatterns(cuts, 100, 0.0);

    std::set<std::string> actual;
    for (auto p : patterns) {
        std::sort(p.begin(), p.end());
        std::string key;
        for (size_t i = 0; i < p.size(); ++i) {
            if (i) key += ",";
            key += std::to_string(p[i]);
        }
        actual.insert(key);
    }

    std::set<std::string> expected = {
        "20,20,20,20,20",
        "20,20,20,40",
        "20,20,60",
        "20,40,40",
        "40,60"
    };

    assert(actual == expected);
    return 0;
}
