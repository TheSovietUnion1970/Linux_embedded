#include <iostream>
#include <vector>
using namespace std;

int nthFibonacciUtil(int n, vector<int>& dp) {
  
    // Base case
    if (n <= 1) {
        return n;
    }

    // Check if the result is
    // already in the dp table
    if (dp[n] != -1) {
        return dp[n];
    }

    // calculate Fibonacci number
    // and store it in dp table
    dp[n] = nthFibonacciUtil(n - 1, dp) 
          + nthFibonacciUtil(n - 2, dp);

    return dp[n];
}
int nthFibonacci(int n) {

    // Create a dp table and 
    // initialize with -1(invalid value)
    vector<int> dp(n + 1, -1);
    
    return nthFibonacciUtil(n, dp);
}

int CountWaysUtil(int n, std::vector<int>& dp){
    // if (n == 1 or n == 2){
    //     return n;
    // } 

    if (n <= 1) return 1;
    else if (n < 0) return 0;

    if (dp[n] == -1) dp[n] = CountWaysUtil(n - 1, dp) + CountWaysUtil(n - 2, dp);

    return dp[n];
}
int CountWays(int n){
    std::vector<int> dp(n, -1);

    return CountWaysUtil(n - 1, dp) + CountWaysUtil(n - 2, dp);
}

int CountWaysUtil3(int n, std::vector<int>& dp){
    // For invalid stair, return 0.
    if (n < 0) return 0;

    // Base case for 0th stair
    if (n <= 1) return 1;

    if (dp[n] == -1) dp[n] = CountWaysUtil3(n - 1, dp) + CountWaysUtil3(n - 2, dp) + CountWaysUtil3(n - 3, dp);

    return dp[n];
}
int CountWays3(int n){
    std::vector<int> dp(n, -1);

    // dp[0] = 0;
    // dp[1] = 1;
    // dp[2] = 2;
    // dp[3] = 4;

    // if (n > 3) return CountWaysUtil3(n - 1, dp) + CountWaysUtil3(n - 2, dp) + CountWaysUtil3(n - 3, dp);
    // else return dp[n];

    return CountWaysUtil3(n - 1, dp) + CountWaysUtil3(n - 2, dp) + CountWaysUtil3(n - 3, dp);
}

int minCostClimbingStairsR(int n, std::vector<int>& v, std::vector<int>& dp){
    if (n == 0 or n == 1){
        return dp[n];
    }

    if (dp[n] == -1) dp[n] = v.at(n) + min(minCostClimbingStairsR(n - 1, v, dp), minCostClimbingStairsR(n - 2, v, dp));

    return dp[n];
}
int minCostClimbingStairs(std::vector<int>& v){
    int n = v.size();
    v.push_back(0);
    std::vector<int> dp(n, -1);

    dp[0] = v.at(0);
    dp[1] = v.at(1);

    return v.at(n) + min(minCostClimbingStairsR(n - 1, v, dp), minCostClimbingStairsR(n - 2, v, dp));
}

// Catalan number
// n = 0, 1, 2, 3, 4, 5… are: 1, 1, 2, 5, 14, 42
// the same for couting unique valid BST
// the same for couting non-crossed lines in Polygon
int numOfValidParenthesis(int n){
    if (n <= 1) return 1;

    int res = 0;
    for (int i = 0; i < n; i++){
        res += numOfValidParenthesis(i)*numOfValidParenthesis(n - i - 1);
    }

    return res;
}

int minSumPathR(vector<vector<int>> &triangle, int i, int j){
    // i -> row, j -> column
    if (i == triangle.size()){
        return 0;
    }

    return triangle[i][j] + min(minSumPathR(triangle, i + 1, j), minSumPathR(triangle, i + 1, j + 1));
}
int minSumPath(vector<vector<int>> &triangle){
    return minSumPathR(triangle, 0, 0);
}

/*
    n = 4, k = 2
    12 34
    13 24
    23 14

    1 234
    2 134
    3 124

    4 123

    BASE: n = 3 (k*Bell_number(k, n - 1)) => '4' joins existing subset 
    12 | 3
    13 | 2
    23 | 1
    ->
    12 | 3
    → 124 | 3
    → 12  | 34

    13 | 2
    → 134 | 2
    → 13  | 24

    23 | 1
    → 234 | 1
    → 23  | 14

    BASE: n = 3 (Bell_number(k - 1, n - 1)) => '4' forms new one
    123
    → 123 | 4

*/
int Bell_number(int k, int n){
    if (k == 1 or n == k) return 1;

    return k*Bell_number(k, n - 1) + Bell_number(k - 1, n - 1);
}



int main() {
    int res;

    // 0, 1, 1, 2, 3, 5, 8, 13, 21
    res = nthFibonacci(5);
    cout << "nthFibonacci(5) = " << res << endl;

    res = CountWays(5);
    cout << "CountWays(5) = " << res << endl;
    // output is 8 -> there are 8 ways to reach top with 5 steps if 1 or 2 steps at a time

	vector<int> cost = {10, 15, 20, 25, 0};
    cout << "minCostClimbingStairs(cost) = " << minCostClimbingStairs(cost) << endl;
    // output is 30 -> there are minimal 30 costs to reach 0 if 1 or 2 steps at a time

    res = numOfValidParenthesis(3);
    cout << "numOfValidParenthesis(3) = " << res << endl;
    // output is 5 with frame = (A)B
    /*
        A = 2 -> ()() or (())
        B = 0
            -> (()()) or ((()))

        A = 1 -> ()
        B = 1 -> ()
            -> ()()

        A = 0
        B = 2 -> ()() or (())
            -> ()()() or ()(())
    */

    vector<vector<int>> triangle{
        {2}, 
        {3, 9}, 
        {1, 6, 7}
    };
    cout << "minSumPath() = " << minSumPath(triangle) << endl;
    // output = 6: 2 -> 3 -> 1, sum = 6

    res = Bell_number(2,4);
    cout << "Bell_number(2,4) = " << res << endl;
    // output = 7
}
