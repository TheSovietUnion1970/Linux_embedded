#include <iostream>
#include <unordered_map>
#include <vector>
 
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
        1. abcda -> 4 = 'abcd'
           abac  -> 2 = 'ab' or 'ac'
        2. unordered_map Val2Idx
            + if Val2Idx[char].count == 1 -> update l = prev if Val2Idx[char] + 1 if (l =< Val2Idx[char] + 1 <= r)
    */
    int NumOfLongestUniqueChar(std::string s){
        int right = 0, left = 0;
        std::unordered_map<char, int> Char2Index;
        int max = 0;
        for (right = 0; right < s.size(); right++){
           
            if (Char2Index.find(s.at(right)) != Char2Index.end()){
                left = Char2Index[s.at(right)] + 1;
            }
 
            Char2Index[s.at(right)] = right;
 
            max = std::max(max, right - left + 1);
        }
 
        return max;
    }
 
    /*
        1. 'aaabbc', k = 2 -> 5 = 'aaabb'
        2. unordered_map Val2Cnt
            + if Val2Idx.size() > k  -> shrink = Val2Idx[char[l]]--
                                            if (Val2Idx[char[l]] == 0) -> erase
                                        l++
    */
    int NumOfkUniqueChar(std::string s, int k){
        int right = 0, left = 0;
        std::unordered_map<char, int> Char2Count;
        int max = 0;
        for (right = 0; right < s.size(); right++){
            Char2Count[s.at(right)]++;
            while (Char2Count.size() > k){
                Char2Count[s.at(left)]--;
                if (Char2Count[s.at(left)] == 0){
                    Char2Count.erase(s.at(left));
                }
 
                left++;
            }
 
            max = std::max(max, right - left + 1);
        }
 
        return max;
    }
 
    /* Specific for k = 2 only for the func above */
    int NumOfTwoUniqueChar1(std::string s){
        int right = 0, left = 0;
        std::unordered_map<char, int> Char2Index;
        int max = 0;
        char first_char, second_char;
 
        for (right = 0; right < s.size(); right++){
            Char2Index[s.at(right)] = right;
 
            if (Char2Index.size() == 1){
                first_char = s.at(right);
            }
            else if (Char2Index.size() == 2){
                second_char = s.at(right);
            }
 
            if (Char2Index.size() == 3){
                if (Char2Index[first_char] > Char2Index[second_char]){
                    left = Char2Index[second_char] + 1;
                    Char2Index.erase(second_char);
 
                    second_char = s.at(right);
                }
                else {
                    left = Char2Index[first_char] + 1;
                    Char2Index.erase(first_char);
 
                    first_char = second_char;
                    second_char = s.at(right);
                }
            }
 
            max = std::max(max, right - left + 1);
        }
 
        return max;
    }
 
 
    /*
        0. Count all substring with <= k distinct
        1. 'a1 a2 a3' -> all possible substring is 1 + 2 + 3 = 6
                                              : '0', '1', '2', '12', '23', '123'
            AtMostK(2):
             + 'aaabb'   -> all possible substring is 1 + 2 + 3 + 4 + 5 = 15 (a group, b group, ab group)

            AtMostK(1):
             + 'aaa'     -> all possible substring is 1 + 2 + 3 = 6 (a group only)
             + 'bb'      -> all possible substring is 1 + 2 = 3 (b group only)
            => ab group only = 15 - (6 + 3) = 6
                                              : 'aaab', 'aab', 'ab', 'aaabb', 'aabb', 'abb'
                                   
        2. Logic is the same as NumOfkUniqueChar
            -> count all (l - r + 1)
    */
    int AtMostK(std::string s, int k){
        std::unordered_map<char, int> Char2Count{0};
        int left = 0, right = 0, count = 0;
        for (char c : s){
            Char2Count[c]++;
            right++;
            while (Char2Count.size() > k){
                Char2Count[s.at(left)]--;
                if (Char2Count[s.at(left)] == 0){
                    Char2Count.erase(s.at(left));
                }
 
                left++;
            }
 
            count += (right - left + 1);
        }
 
        return count;
    }
 
    /* ExactK =  AtMostK(k) - AtMostK(k-1) */
    int ExactK(std::string s, int k){
        return AtMostK(s, k) - AtMostK(s, k - 1);
    }
 

    int MaxEraseVal(std::vector<int>& nums){
        int right = 0, left = 0;
        std::unordered_map<int, int> Val2Idx;
        int max = 0, nax_score = 0, tmp = 0;
        for (int x : nums){
           
            // take 5,2,1,2,5 to check Val2Idx[x] > left => Val2Idx[x] must be bw left and right
            if (Val2Idx.count(x) && Val2Idx[x] >= left){
                left = Val2Idx[x] + 1;
                Val2Idx.erase(x);
            }
 
            Val2Idx[x] = right;
           
            std::cout << "[l, r]: " << left << ", " << right << std::endl;
 
            tmp = 0;
            for (int i = left; i <= right; i++){
                tmp += nums.at(i);
            }
            nax_score = std::max(nax_score, tmp);
 
            right++;
        }
 
        return nax_score;
    }
 
    int NumOfLongestNiceString(std::vector<int>& nums){
        int right = 0, left = 0;
        int max_len = 0;
        for (right = 1; right < nums.size(); right++){
 
            if (nums.at(right - 1) & nums.at(right) != 0){
                left = right;
            }
 
            max_len = std::max(max_len, right - left + 1);
        }
 
        return max_len;
    }
 
    int MaxSumOfKLength(std::vector<int>& nums, int k){
        int right = 0, left = 0;
        int max_sum = 0, tmp_sum = 0;
        std::unordered_map<int, int> Val2Cnt;
 
        // init
        for (int i = 0; i < nums.size(); i++){
            Val2Cnt[nums.at(right)]++;
            tmp_sum += nums.at(right);
 
            // check k subarray has k distinct elements
            if ((right - left + 1) == k and Val2Cnt.size() == k){
                max_sum = std::max(max_sum, tmp_sum);
                Val2Cnt.erase(nums.at(left));
            }
 
            // counted if k is got
            if ((right - left + 1) == k) {
                tmp_sum -= nums.at(left);
                left++;
            }
 
            right++;
 
 
        }
 
        return max_sum;
    }
 
    /*
        0. Max sum of k distict + contiguous subarray
            Ex with {1,1,2,3,3,4,4} k = 2:
            1,1 -> not take
            1,2 -> take + cpr with max
            2,3 -> take + cpr with max
            ...
            3,4 -> take + cpr with max
            4,4 -> not take

        1. {1,1,2,3,3,4,4}:
            + k = 1 -> 4 = {4}
            + k = 2 -> 7 = {3,4}
            + k = 3 -> 6 = {1,2,3}
            + k = 4 -> 0 

           K distinct with K-sized subarray
           -> Val2Cnt.size() = k

        2. unordered_map Val2Cnt
            + if Val2Idx.size() > k  -> shrink until (l - r + 1) > k becoming false
            + After this, we have [l,r] for subarray with k elements
                -> then only check k elements has k distinct by if Val2Cnt.size() == k
    */
    int MaxSumOfKLengthContiguousOp1(std::vector<int>& nums, int k){
        int right = 0, left = 0;
        int max_sum = 0, tmp_sum = 0;
        std::unordered_map<int, int> Val2Cnt;
 
        // init
        for (int i = 0; i < nums.size(); i++){
            Val2Cnt[nums.at(right)]++;
            tmp_sum += nums.at(right);
 
            // this while -> shrink until  right - left + 1 > k
            while (right - left + 1 > k){
                tmp_sum -= nums.at(left);
                Val2Cnt[nums.at(left)]--;
                if (Val2Cnt[nums.at(left)] == 0){
                    Val2Cnt.erase(nums.at(left));
                }
 
                //max_sum = std::max(tmp_sum, max_sum);
                left++;
            }
 
            // only updaTE if k distinct elements
            if (Val2Cnt.size() == k) max_sum = std::max(tmp_sum, max_sum);
 
            right++;
        }
 
        return max_sum;
    }
 
    /*
        0. Count all substring with k pairs
            + {1,2,1,2}
               0 0 1 1 -> 2 pairs
            + {1,1,1}
               0 1 2   -> 3 pairs

        1. {1,1,2,2,3,3}, k = 2 -> 5
                        + {1,1,2,2}, {1,1,2,2,3}, {1,1,2,2,3,3}
                        + {1,2,2,3,3}
                        + {2,2,3,3}
                                   
        2. loop until pairs >= k
            while (pairs >= k) -> max += size() - r
                                  l++
    */
    int MaxOfKpairSubArrays(std::vector<int>& nums, int k){
        int right = 0, left = 0;
        int max_sum = 0, pairs = 0;
        std::unordered_map<int, int> Val2Cnt;
        for (right = 0; right < nums.size(); right++){
            pairs += Val2Cnt[nums.at(right)];
            Val2Cnt[nums.at(right)]++;
 
            while(pairs >= k){
                //std::cout << right << ", " << nums.size() - right << std::endl;
                max_sum += (nums.size() - right);
 
                Val2Cnt[nums.at(left)]--;
                pairs -= Val2Cnt[nums.at(left)];
                left++;
            }
        }
 
        return max_sum;
    }
 
    /* Limit of k pairs and k_max pair

        1. {1,1,2,2,3,3}, k = 2, kmax = 3 -> 4
                        + {1,1,2,2}, {1,1,2,2,3}
                        + {1,2,2,3,3}
                        + {2,2,3,3}
    */
    int Solution_NumOfKSimillarElementSubArray(std::vector<int>& nums, int k, int k_max){
        int k_val = MaxOfKpairSubArrays(nums, k);
        int km_val = MaxOfKpairSubArrays(nums, k_max);
 
        //std::cout << "k = " << k_val << ", k_max = " << km_val << std::endl;
        return k_val - km_val;
    }
};
 
int main(){
    WindowSliding ws("Window Sliding");
    std::vector<int> v = {1,1,2,3,3,4,4};
    // int num = t1.Solution_NumOfKSimillarElementSubArray(v, 3, 4);

    int max = ws.NumOfLongestUniqueChar("abcda");
    std::cout << "NumOfLongestUniqueChar: Max = " << max << std::endl;

    max = ws.NumOfkUniqueChar("aaabbc", 2);
    std::cout << "NumOfkUniqueChar: Max = " << max << std::endl;

    max = ws.ExactK("aaabb", 2);
    std::cout << "ExactK: Max = " << max << std::endl;

    max = ws.MaxSumOfKLengthContiguousOp1(v, 3);
    std::cout << "MaxSumOfKLengthContiguousOp1: Max = " << max << std::endl;

    max = ws.MaxOfKpairSubArrays(v, 2);
    std::cout << "MaxOfGoodSubArrays: Max = " << max << std::endl;

    max = ws.Solution_NumOfKSimillarElementSubArray(v, 2, 3);
    std::cout << "Solution_NumOfKSimillarElementSubArray: Max = " << max << std::endl;
 
    return 0;
}
