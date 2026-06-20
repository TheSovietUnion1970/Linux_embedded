#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v2 = {10, 20, 30, 40};

    // Example 1: Copy
    for (auto x : v2) {
        x = 999;           // Only changes the copy
    }
    cout << v2[0] << endl;   // Still prints 10

    // Example 2: Reference
    for (auto &x : v2) {
        x = 999;           // Changes the actual element in vector
    }
    cout << v2[0] << endl;   // Now prints 999
}