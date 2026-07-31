// Iterator invalidation lab - the FIXED patterns (broken ones live in comments).
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 0);              // 0..9

    // Task 1: remove odd numbers while iterating.
    // BROKEN: for (auto it=v.begin(); it!=v.end(); ++it) if (*it%2) v.erase(it);
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2) it = v.erase(it);             // erase returns next valid
        else         ++it;
    }
    // or simply: std::erase_if(v, [](int x){ return x % 2; });

    // Task 2: append while iterating - by INDEX against a captured size,
    // because push_back may reallocate and kill every iterator/reference.
    // BROKEN: for (int x : v) v.push_back(x);     // UB on reallocation
    const size_t n = v.size();
    v.reserve(v.size() * 2);                       // belt AND suspenders
    for (size_t i = 0; i < n; ++i) v.push_back(v[i]);

    // Task 3: the reference that dies. BROKEN version:
    //   int& first = v[0]; v.push_back(99); std::cout << first;  // maybe UB
    // FIXED: re-acquire after any potentially-reallocating call, or reserve.

    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";
    return 0;
}
