#include <iostream>
#include <string>
#include <stack>
using namespace std;

#define SQUARE(x) (x * x)
constexpr int square(int x) { return x * x; }

int main() {
    int result = square(3 + 2);   // Expands to: (3 + 2 * 3 + 2) → Wrong!
    cout << result << endl;               // Output: 11 (should be 25)
}


//                           macro     constexpr
// CHECK compile             No        Yes
// calculation com (!5)      No        Yes
// Math issue (square)       Yes       No     