#include <iostream>
#include <vector>
#include <climits>

// Max of 2 non-adjacent num in arr
int MaxRobR(std::vector<int> house, int i, std::vector<int>& dp){
    if (i < 0){
        return 0;
    }

    if (dp[i] == -1){
        int op1, op2;
        op1 = house[i] + MaxRobR(house, i - 2, dp);
        op2 = MaxRobR(house, i - 1, dp);

        dp[i] = std::max(op1, op2);
    }

    return dp[i];
}
int MaxRob(std::vector<int> house){
    int n = house.size();
    std::vector<int> dp(n, -1);

    return MaxRobR(house, n - 1, dp);
}
int MaxRob1(std::vector<int> house){
    int n = house.size();
    std::vector<int> dp(n, -1);
    dp[0] = house[0];
    dp[1] = std::max(house[0], house[1]);

    for (int i = 2; i < n; i++){
        dp[i] = std::max(house[i] + dp[i - 2], dp[i - 1]);
    }

    return dp[n - 1];
}

/*
sum = 5, coin = {2} => 0

              5 (2)
             /     \
          3 (2)     5 ()
         /    \
      1 (2)    3 ()
     /    \
  -1 (2)   1 ()


sum = 0 or n = 0 => 0

==================================================

sum = 2, coin = {1} => 1 including {1,1}

              2 (1)
             /     \
          1 (1)     2 ()
         /    \
      0 (1)    1 ()

0 (1) = 1


==================================================

sum = 3, coin = {1,2} => 2 including {1,1,1}, {1,2}

                      3 (1,2)
                   /           \
              1 (1,2)           3 (1)
             /       \          /      \
        -1 (1,2)    1 (1)      2 (1)    3 ()
                    /   \      /   \
                  0(1)  1()  1(1)  2()
                             /   \
                           0(1)  1()

0 (1) = 1
*/
// (sum - coins[i], i) + (sum, i - 1)
// i is not decreased as value usage is infinite
int countCoinsR(std::vector<int> coins, int sum, int i){

    if (sum == 0) return 1;
    if (i < 0 or sum < 0) return 0;

    return countCoinsR(coins, sum - coins[i], i) + countCoinsR(coins, sum, i - 1);
}
int countCoins(std::vector<int> coins, int sum){
    int n = coins.size();
    return countCoinsR(coins, sum, n - 1);
}

int findMinCostR(std::vector<std::vector<int>> cost, int i, int j){
    int row = cost.size();
    int col = cost[0].size();

    if (i == row or j == col) return INT_MAX; // make sure that invalid path has larger than vald one
    if (i == row - 1 and j == col - 1) return cost[i][j];

    return cost[i][j] + std::min(findMinCostR(cost,i+1,j), 
                                 std::min(findMinCostR(cost,i,j + 1), findMinCostR(cost,i+1,j+1)));
}
int findMinCost(std::vector<std::vector<int>> cost){

    return findMinCostR(cost, 0,0);
}

// (sum - coins[i], i - 1) + (sum, i - 1)
bool subsetSumRec(std::vector<int> arr, int sum, int i){
    if (sum == 0) return true;

    if (sum < 0 or i < 0) return false;

    // for the first element
    if (i == 0){
        if (sum == arr[0]) return true;
        else return false;
    }

    return subsetSumRec(arr, sum - arr[i], i - 1) or subsetSumRec(arr, sum, i - 1);
}
bool isSubsetSumRec(std::vector<int> arr, int sum){
    int n = arr.size();

    return subsetSumRec(arr, sum, n - 1);
}

/*
    NumOfPartition(4) = 8
    '1234'
    ->
    1  234 -> 1 2 3 4
              1 2 34
        
              1 23 4
            
              1 234
            
    12  34 -> 12 34
              12 3 4
            
    123 4  -> 123 4

    1234   -> 1234
*/
int NumOfPartitionR(int n_first, int n_remaining){
    if (n_remaining <= 1) return 1;

    int rett = 0;
    for (int i = 1; i <= n_remaining; i++){
        rett += NumOfPartitionR(i, n_remaining - i);
    }

    return rett;
}
int NumOfPartition(int n){
    return NumOfPartitionR(1, n);
}

/*
    abcd -> for cd, we can find num of b - cd or ab - cd

              *
    decodeR("123", 1) = 2 including {2, 3}, {23}

             *
    decodeR("123", 0) = 3 including {1, 2, 3}, {1, 23}, {12, 3}
*/
int decodeR(std::string digit, int i){
    int n = digit.length();

    if (i >= n) return 1;

    int way = 0;
    // 1 digit
    way = decodeR(digit, i + 1);

    // 2 digits
    if (i + 1 < n 
        and 
        (digit[i] == '1' and digit[i + 1] <= '9') or
         digit[i] == '2' and digit[i + 1] <= '6'
       ) {
        way += decodeR(digit, i + 2);
    }

    return way;
}
int decode(std::string digit){
    return decodeR(digit, 0);
}

/*
                                                       {1,1,1}         {1,2}
                      3 (1,2)                            3 (1)          1 (1,2)   
                   /           \                          /               \
              1 (1,2)           3 (1)                    2 (1)           1 (1)        
             /       \          /      \                  /               /       
        -1 (1,2)    1 (1)      2 (1)    3 ()             1(1)            0(1)
                    /   \      /   \                     /
                  0(1)  1()  1(1)  2()                  0(1)
                             /   \                  -> depth = 3      -> depth = 2
                           0(1)  1()
*/
int minCoinsRecur(std::vector<int> coins, int sum, int i){

    if (sum == 0) return 0;
    if (i < 0 or sum < 0) return INT_MAX;

    int take, no_take;

    take = minCoinsRecur(coins, sum - coins[i], i); 
    if (take != INT_MAX) take++;

    no_take = minCoinsRecur(coins, sum, i - 1);

    return std::min(take, no_take);
}
int minCoins(std::vector<int> coins, int sum){
    int n = coins.size();
    return minCoinsRecur(coins, sum, n - 1);
}

int main(){
    int res;
    bool ret;

    std::vector<int> house = {5, 3, 4, 11, 2};
    res = MaxRob1(house);
    std::cout << "MaxRob = " << res << std::endl;
    // output is 16 = 5 + 11 

    std::vector<int> coins = {1, 2};
    res = countCoins(coins, 3);
    std::cout << "countCoins = " << res << std::endl;
    // output is 2, including {1,1,1} and {1,2}

    std::vector<std::vector<int>> cost = {
        {1,2,3},
        {4,8,2},
        {1,5,3}
    };
    res = findMinCost(cost);
    std::cout << "findMinCost = " << res << std::endl;
    // output is 8, 1 -> 2 -> 2 -> 3

    std::vector<int> arr = {3, 34, 4, 12, 5, 2};
    ret = isSubsetSumRec(arr, 39);
    std::cout << "isSubsetSumRec = " << ret << std::endl;
    // output is true as 39 = subset(34, 5);

    res = NumOfPartition(3);
    std::cout << "NumOfPartition = " << res << std::endl;
    // output is 4

    res = decode("1212");
    std::cout << "decode = " << res << std::endl;
    // output is 8

    std::vector<int> coins1 = {1, 2};
    std::cout << "minCoins = " << minCoins(coins1, 3) << std::endl;
    // output is 2
}
