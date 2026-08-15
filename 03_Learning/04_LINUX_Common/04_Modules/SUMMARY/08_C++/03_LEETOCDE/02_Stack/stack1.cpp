#include <iostream>
#include <vector>
#include <stack>
#include <algorithm> // Required for std::reverse
using namespace std;

void printVector(std::vector<int> s){
    for (int v : s){
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

class StackOperation {
public:
    string removePair(string s)
    {
        int i = 0;
        while (i < s.size() - 1 and s.size() != 0)
        {
            if (s.at(i + 1) == s.at(i))
            {
                s.erase(i, 1);
                s.erase(i, 1);
            }
            else
            {
                i++;
            }
        }

        if (s.size() == 0)
            return "-1";
        return s;
    }

    string removePair2(string s)
    {
        std::stack<char> stk;
        std::string out;
        for (char c : s)
        {
            if (!stk.empty() and c == stk.top())
            {
                stk.pop();
            }
            else
            {
                stk.push(c);
            }
        }

        if (stk.empty())
            return "-1";
        else
        {
            while (!stk.empty())
            {
                out.push_back(stk.top());
                stk.pop();
            }

            reverse(out.begin(), out.end());

            return out;
        }
    }

    vector<int> calculateSpan2(vector<int> arr)
    {
        std::stack<int> stk;
        int n = arr.size();
        std::vector<int> span(n, 0);

        for (int i = 0; i < n; i++)
        {
            while (!stk.empty() and arr[stk.top()] <= arr[i])
            {
                stk.pop();
            }

            if (stk.empty())
            {
                span[i] = i + 1;
            }
            else
                span[i] = i - stk.top();

            stk.push(i);
        }

        return span;
    }

    int maxWater(vector<int> arr)
    {
        int n = arr.size();

        // left[i] contains height of tallest bar to the
        // left of i'th bar including itself
        vector<int> left(n);

        // right[i] contains height of tallest bar to
        // the right of i'th bar including itself
        vector<int> right(n);

        int res = 0;

        // fill left array
        left[0] = arr[0];
        for (int i = 1; i < n; i++)
            left[i] = max(left[i - 1], arr[i]);
        //printVector(left);

        // fill right array
        right[n - 1] = arr[n - 1];
        for (int i = n - 2; i >= 0; i--)
            right[i] = max(right[i + 1], arr[i]);
        //printVector(right);

        // calculate the accumulated water element by element
        for (int i = 1; i < n - 1; i++)
        {
            int minOf2 = min(left[i], right[i]);
            res += minOf2 - arr[i];
        }

        return res;
    }

    int sumSubarrayMins2(vector<int> arr){
        int n = arr.size();
        std::vector<int> l(n, 0), r(n, 0);
        std::stack<int> stkL, stkR;

        for (int i = 0; i < n; i++){
            while(!stkL.empty() and arr[i] < arr[stkL.top()]){
                stkL.pop();
            }
            if (!stkL.empty()) l[i] = i - stkL.top();
            else l[i] = i + 1;

            stkL.push(i);
        }
        //PrintVector(l);

        for (int i = n - 1; i >= 0; i--){
            while(!stkR.empty() and arr[i] < arr[stkR.top()]){
                stkR.pop();
            }
            if (!stkR.empty()) r[i] =  stkR.top() - i;
            else r[i] = n - i;

            stkR.push(i);
        }
        //PrintVector(r);

        int sum = 0;
        for (int i = 0; i < n; i++){
            sum += arr[i]*l[i]*r[i];
        }

        return sum;

    } 
};

int main() {
    std::string out_str;
    std::vector<int> v = {1,5,3,6,8,4};
    std::vector<int> out_v;
    int max;
    StackOperation s;

    // output = "ac"
    out_str = s.removePair2("aaabbccc");
    std::cout << "removePair2: out_str = " << out_str << std::endl;

    // output 1 2 1 4 5 1
    out_v = s.calculateSpan2(v);
    std::cout << "calculateSpan2: out_v: ";   
    printVector(out_v);

    // output is 2
    max = s.maxWater(v);
    std::cout << "maxWater: max = " << max << std::endl;

    // output is 17
    std::vector<int> v1 = {3,1,2,4};
    max = s.sumSubarrayMins2(v1);
    std::cout << "sumSubarrayMins2: max = " << max << std::endl;
}
