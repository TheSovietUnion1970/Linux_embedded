#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

void SortedByStack(vector<int>& A){
    stack<int> stk_in;
    stack<int> stk_out;
    int sz;

    for (int i : A){
        stk_in.push(i);
    }

    while(!stk_in.empty()){
        int cur = stk_in.top();
        stk_in.pop();

        while(!stk_out.empty() and cur > stk_out.top()){
            stk_in.push(stk_out.top());
            stk_out.pop();
        }

        stk_out.push(cur);
    }

    A.clear();
    sz = stk_out.size();
    for (int i = 0; i < sz; i++){
        A.insert(A.begin() + i, (int)stk_out.top());
        stk_out.pop();
    }

}

int CoutBrackets(string A){
    stack<char> opened_bracket;
    stack<char> closed_bracket;

    for (char c : A){
        if (c == '(') {
            opened_bracket.push(c);
        }
        else if (c == ')'){
            if (!opened_bracket.empty()){
                opened_bracket.pop();
            }
            else {
                closed_bracket.push(c);
            }
        }
    }

    return opened_bracket.size() + closed_bracket.size();
}

vector<int> nextLargerElement(vector<int>& A) {
    stack<int> stk;
    vector<int> out;

    // init
    for (int i = 0; i < A.size(); i++){
        out.push_back(-1);
    }
    //stk.push(A.at(A.size() - 1));

    for (int i = A.size() - 1; i >= 0; i--){
        while(!stk.empty() and A.at(i) > stk.top()){
            stk.pop();
        }
        if (stk.empty()) out.at(i) = -1;
        else out.at(i) = stk.top();

        stk.push(A.at(i));
    }

    return out;
}

void PrintVector(vector<int> A){
    for (int i : A){
        cout << i << " ";
    }
    cout << endl;
}

int main() {
    vector<int> A = {1, 3, 2, 4}; // -> 8, -1, 1, 3, -1
    PrintVector(nextLargerElement(A));
    return 0;
}