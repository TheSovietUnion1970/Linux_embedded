#include <iostream>
#include <unordered_map>
#include <vector>
#include <climits>
#include <unordered_set>
using namespace std;
 
class WindowSliding {
private:
    std::string name;
public:
    /*
        l = left
        r = right
    */
    WindowSliding (std::string n) : name(n){}

    /*
        0. Maximum len of equal sub arr when removing kth elements
            + {1,2,3,1,2,3,1}, k = 4
            -> {1,1,1}, max len = 3 -> remove 2,3,2,3
            + {1,2,3,1,2,3,1}, k = 3
            -> {1,1}, max len = 2 -> remove first 2,3 or second 2,3

        1. {1,2,3,1,2,3,1}
        -> we gather all same number into ordered index:
            + {1,1,1}
            + {2,2}
            + {3,3}
            -> max len = 3, {1,1,1}
                                    
        2. The substract of real idx and same array idx
            + {1,2,3,1,2,3,1}
            0     3
            + {1,1,1}
            0 1

            => 3 - 0 - (1 - 0) = 2 -> 2 here is the len {2,3} between 1 value

    */
    int MaxLen(std::vector<int>& vec, int k_removed){
        int l = 0, r = 0;
        std::unordered_map<int, std::vector<int>> Val2Vector;
        int max_len = 0;

        for (int i = 0; i < vec.size(); i++){
            Val2Vector[vec.at(i)].push_back(i);
        }

        for (auto pair : Val2Vector){
            std::vector<int> vec = pair.second;

            for (r = 0; r < vec.size(); r++){
                while (vec[r] - vec[l] - (r - l) > k_removed){
                    l++;
                }

                max_len = std::max(r - l + 1, max_len);
            }
        }

        return max_len;
    }
 
    /*
    0. With at most m subarray, len = [l,r] -> Find the max sum of at most m subarray
        + {-1, 7,-4}, m=1, l=2, r=3 
            -> max sum = 6, 
            -> {-1,7}

        + {1, 2, 3, 4, 5}, m=2, l=1, r=3
            -> max sum = 15, 
            -> ({1, 2}, {3, 4, 5}) or ({1, 2, 3}, {4, 5})

    CASE 1:
        DP Table: Maximum Sum using first i elements with exactly j subarrays
        (l=1, r=3, m=2)

        nums   = [1, 2, 3, 4, 5]
        prefix = [0, 1, 3, 6, 10, 15]

                            <-------- m ------->
                            j=0     j=1      j=2
        |  i=0  []             0      -        -
        |  i=1  [1]            0      1        -
        n  i=2  [1,2]          0      3        3
        +  i=3  [1,2,3]        0      6        6
        1  i=4  [1,2,3,4]      0      10       10
        |  i=5  [1,2,3,4,5]    0      15       15   ← Answer


    CASE 2:
        nums = [-1, 7,-4], m=1, l=2, r=3
        prefix [ 0,-1, 6, 2]
        +---------------+-------+-------+
        | i \ j         |  j=0  |  j=1  |
        +---------------+-------+-------+
        | 0  []         |   0   |   -   |
        | 1  [-1]       |   0   |  -1   |
        | 2  [-1,7]     |   0   |   6   |
        | 3  [-1,7,-4]  |   0   |   2   |
        +---------------+-------+-------+

    */
    int MaximumSum(vector<int>& nums, int m, int l, int r){
        int n = nums.size();
        std::vector<int> prefix(n+1, 0);
        //std::vector<int, std::vector<int>> Vec2MaxSumPerSubarray;
        int Vec2MaxSumPerSubarray[n+1][m+1] = {INT_MIN};
        
        prefix[0] = 0;
        for (int i = 1; i <= n; i++){
            prefix[i] = prefix[i-1] + nums.at(i-1);
        }

        Vec2MaxSumPerSubarray[0][0] = 0;
        for (int i = 1; i <= n; i++){
            Vec2MaxSumPerSubarray[i][0] = 0;
            for (int j = 1; j <= std::min(i,m); j++){
                
                for (int len = l; len <= r; len++){
                    if (i-len >= 0){
                        int subsum = prefix[i] - prefix[i-len];
                        int sum = subsum + Vec2MaxSumPerSubarray[i-len][j-1];
                        // std::cout << subsum << std::endl;
                        Vec2MaxSumPerSubarray[i][j] = std::max(sum, Vec2MaxSumPerSubarray[i][j]);

                        // also compare with the previous j-1
                        Vec2MaxSumPerSubarray[i][j] = std::max(Vec2MaxSumPerSubarray[i][j], Vec2MaxSumPerSubarray[i-1][j]);
                    }
                    //std::cout << Vec2MaxSumPerSubarray[i][j] << std::endl;
                }

            }
        }

        int res = INT_MIN;
        for (int j = 1; j <= m; j++){
            // std::cout << Vec2MaxSumPerSubarray[n][j] << std::endl;
            res = std::max(Vec2MaxSumPerSubarray[n][j], res);
        }

        return res;
    }

    /*
        0. Find the longest same string (even duplicated each other)
            + "banana" -> "ana"
            + "abcabc" -> "abc"

        1. + Use unordered_set to add substring, then check count > 0
           + Use binary search
             Ex: 8 bytes string with 3 bytes same
                  + Check 4 bytes, if no string found -> then 2 bytes
                  + 2 bytes are found -> Find longer 3 bytes

    */
    std::string FindDuplicateKLength(const string& s, int k_len){
        std::unordered_set<std::string> seen_string;
        int n = s.size();

        for (int i = 0; i < s.size() - k_len + 1; i++){
            std::string sub_string = s.substr(i, k_len);
            if (seen_string.count(sub_string)){
                //std::cout << "Duplicates: " << sub_string << ", num = " << seen_string.count(sub_string) << std::endl;
                return sub_string;
            }
            seen_string.insert(sub_string);
        }

        return "";
    }

    std::string LongestDuplicatedString(const string& s){
        int left = 1;
        int right = s.length() - 1;
        std::string out;

        while (left <= right){
            int mid = left + (right - left)/2;
            std::string tmp;

            tmp = FindDuplicateKLength(s, mid);

            if (tmp.empty()){
                right = mid - 1;
            }
            else {
                out = tmp;
                left = mid + 1;
            }
        }

        return out;
    }

    /*
        0. Find the max sum of 2 non-overlapping arrs
            + {-1,2,-1,-1,3,4}, fl = 1, sl = 2 -> max sum = 2+3+4 = 9

        1. with fl = 1 left, sl = 2 right
            + {[-1], [2, -1]} -> first_max = -1, max_sum = 0
            + {[2] , [-1,-1]} -> first_max = 2,  max_sum = 0
            ...
            + {[-1], [3,  4]} -> first_max = 2,  max_sum = 9
           with sl = 2 left, fl = 1 right

    */
    int maxSumTwoNoOverlap(vector<int>& nums, int firstLen, int secondLen){
        int n = nums.size();
        std::vector<int> prefix(n+1, 0);
        int first_sum = INT_MIN, total_sum = INT_MIN;

        //prefix[0] = 0;
        for (int i = 1; i < n+1; i++){
            prefix[i] = prefix[i-1] + nums.at(i-1);
        }

        // first len left, right len right
        for (int i = firstLen; i < n - secondLen + 1; i++){
            first_sum = std::max(first_sum, prefix[i] - prefix[i - firstLen]);
            total_sum = std::max(total_sum, first_sum + prefix[i + secondLen] - prefix[i]);
        }

        // second len left, first len right
        for (int i = secondLen; i < n - firstLen + 1; i++){
            first_sum = std::max(first_sum, prefix[i] - prefix[i - secondLen]);
            total_sum = std::max(total_sum, first_sum + prefix[i + firstLen] - prefix[i]);
        }

        return total_sum;
    }
};
 
int main(){
    WindowSliding ws("Window Sliding");

    std::vector<int> v = {1,2,3,1,2,3,1};
    int max = ws.MaxLen(v,3);
    std::cout << "MaxLen = " << max << std::endl;

    std::vector<int> v1 = {-1,7,-4};
    max = ws.MaximumSum(v1, 1,2,3);
    std::cout << "MaximumSum = " << max << std::endl;

    std::string out = ws.LongestDuplicatedString("banana");
    std::cout << "LongestDuplicatedString = " << out << std::endl;

    std::vector<int> v2 = {-1,2,-1,-1,3,4};
    max = ws.maxSumTwoNoOverlap(v2, 1,2);
    std::cout << "MaximumSum = " << max << std::endl;
    return 0;
}
