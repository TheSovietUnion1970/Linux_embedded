#include <iostream>
#include <vector>
#include <unordered_map>
#include <climits>
#include <algorithm>
#include <unordered_set>
using namespace std;

class Window_Sliding
{
private:
    std::string name;

public:
    /*
        l = left
        r = right
    */
    Window_Sliding (std::string n) : name(n){}

    /*
        0. Find the max munber of subarrays with k arr[k] < arr[k+1] > arr[k+2] ....
            + {3,1,2,-1,5,5,5}, k = 3 -> 6 
                + {3,1,2}, {3,1,2,-1}, {3,1,2,-1,5}
                + {1,2,-1}, {1,2,-1,5}
                + {2,-1,5}

        1. window sliding 
    */
    int maxTurbulenceSize(vector<int> &arr, int k)
    {
        int r = 0, l = 0;
        int n = arr.size();
        int max_num = 0;

        for (r = 2; r < n; r++)
        {
            if (arr.at(r) > arr.at(r - 1) and arr.at(r - 1) < arr.at(r - 2))
            {
            }
            else if (arr.at(r) < arr.at(r - 1) and arr.at(r - 1) > arr.at(r - 2))
            {
            }
            else
            {
                l = r - 1;
            }

            if (r - l + 1 >= k)
            {
                std::cout << l << " -> " << r << std::endl;
                max_num += (r - l + 1) - k + 1;
            }
        }

        return max_num;
    }

    /*
        0. Minimum swap to make arr balanced
            + {1,1,0,0} -> minimum of swap is 1 -> {1,0,1,0}

    */
    int minSwaps2(string s)
    {
        int n = s.length();
        int count0 = 0, count1 = 0;
        for (char c : s)
        {
            if (c == '0')
                count0++;
            else
                count1++;
        }
        // Impossible cases
        if (abs(count0 - count1) > 1)
            return -1;
        // Case 1: Target starts with '0' (010101...)
        int swaps0 = 0;
        for (int i = 0; i < n; i++)
        {
            char expected = (i % 2 == 0) ? '0' : '1';
            if (s[i] != expected)
                swaps0++;
        }
        swaps0 /= 2; // Each swap fixes 2 mismatches
        // Case 2: Target starts with '1' (101010...)
        int swaps1 = 0;
        for (int i = 0; i < n; i++)
        {
            char expected = (i % 2 == 0) ? '1' : '0';
            if (s[i] != expected)
                swaps1++;
        }
        swaps1 /= 2;
        // Choose the valid minimum
        if (count0 == count1)
        {
            return min(swaps0, swaps1);
        }
        else if (count0 > count1)
        {
            // Must start with '0'
            return swaps0;
        }
        else
        {
            // Must start with '1'
            return swaps1;
        }
    }

    /*
        0. Find the max munber of subarrays with k incremented
            + {1,2,3,4,8,10,12} -> 4
                + {1,2,3}, {1,2,3,4}, {2,3,4}
                + {8,10,12}

        1. window sliding 
    */
    int NumOfIncrementedByKSubArr(std::vector<int> s)
    {
        int n = s.size();
        int num = 0;
        int l = 0;

        for (int r = 2; r < n; r++)
        {
            int diff1 = s.at(r) - s.at(r - 1);
            int diff2 = s.at(r - 1) - s.at(r - 2);

            if (diff1 == diff2)
            {
                // num += r - l - 1;
            }
            else
            {
                l = r - 1; // make sure difference bw l and r next loop is 2
            }

            num = std::max(num, r - l + 1);
        }

        return num;
    }
};

int main()
{
    Window_Sliding ws("Window Sliding");

    std::vector<int> v = {3,1,2,-1,5,5,5};
    int max = ws.maxTurbulenceSize(v,3); // output is 6
    std::cout << "maxTurbulenceSize = " << max << std::endl;

    max = ws.minSwaps2("1100"); // output is 6
    std::cout << "minSwaps2 = " << max << std::endl;

    std::vector<int> v1 = {1,2,3,4,8,10,12};
    max = ws.NumOfIncrementedByKSubArr(v1); // output is 4
    std::cout << "NumOfIncrementedByKSubArr = " << max << std::endl;
}

