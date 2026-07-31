#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: words <file>\n"; return 1; }

    std::ifstream in(argv[1]);                    // RAII: closes itself
    if (!in) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }

    std::unordered_map<std::string, int> freq;    // Dictionary<string,int>
    std::string word;
    while (in >> word) {
        // normalize: lowercase, strip non-alpha edges
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.front())))
            word.erase(word.begin());
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.back())))
            word.pop_back();
        if (!word.empty())
            ++freq[word];              // operator[] inserting IS wanted here
    }

    // map -> vector of pairs, because maps can't be sorted by value
    std::vector<std::pair<std::string, int>> ranked(freq.begin(), freq.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;        // tie-break: alphabetical
              });

    std::erase_if(ranked, [](const auto& p) { return p.second < 2; });  // C++20
    // pre-C++20: ranked.erase(std::remove_if(...), ranked.end());

    const size_t top = std::min<size_t>(10, ranked.size());
    for (size_t i = 0; i < top; ++i)
        std::cout << ranked[i].first << ": " << ranked[i].second << "\n";
    return 0;
}
