#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <unordered_map>
#include <stack>

class StackOperation {
public:
    /*
        1. IN : -1 1 3 5
           OUT:

           IN : -1 1 3 
           OUT: 5

           IN : -1 1 5   -> push 5 back to IN, then push 3 to OUT
           OUT: 3
           ....
           -> OUT is always in sorted order
        2. Use stack IN and OUT
    */
    std::vector<int> Sorted(std::vector<int> s){
        std::stack<int> in;
        std::stack<int> out;
        std::vector<int> out_vec;
        int tmp_size = 0;

        for (int v : s){
            in.push(v);
        }

        tmp_size = in.size();
        while(!in.empty()){
            int val = in.top();
            in.pop();

            while (!out.empty() and out.top() < val){
                in.push(out.top());

                out.pop();
            }

            out.push(val);
        }

        while(!out.empty()){
            out_vec.push_back(out.top());
            out.pop();
        }

        return out_vec;
    }

    /*
        0. This algo is used for parsing a math string
        1. a+b*c -> abc*+ 
           a+b*c-d -> abc*+d- 
           (a+b)*c -> ab+c*
        2. stack: follow: if prec(c) <= prec(stk.top()) -> pop all
            + c = '*', top = '+' -> stack: '+', '*'
            + c = '-', top = '*' -> pop all, stack: '-'
    */
    std::string ParsedAlo(std::string s){
        std::stack<char> stk;
        std::string out;

        auto prec=[](char c)-> int{
            if (c == '+' or c == '-') return 1;
            else if (c == '*' or c == '/') return 2;
            else return -1;
        };

        for (char c : s){
            if (c!='+' and c!='-' and c!='*' and c!='/' and c!='(' and c!=')'){
                out.push_back(c);
            }

            else if (c == '(') stk.push(c);
            else if (c == ')'){
                while(!stk.empty() and stk.top() != '('){
                    out.push_back(stk.top());
                    stk.pop();     
                }

                stk.pop();   // remove '('
            }

            else {
                while (!stk.empty() and prec(c) <= prec(stk.top())){
                    out.push_back(stk.top());
                    stk.pop();
                }
                stk.push(c);
            }
        }

        while (!stk.empty()){
            out.push_back(stk.top());
            stk.pop();
        }

        return out;
    }

    /*
        0. This algo is used for checking valid brackets
        1. ({[]}) -> true
           O_stk: (
           C_stk:

           O_stk: ({
           C_stk:

           O_stk: ({[
           C_stk:

           O_stk: ({[    -> pop [] in both stk  -> O_stk: ({
           C_stk: ]                             -> O_stk:

    */
    bool CheckValidBracketString(std::string s){
        std::stack<char> opened_bracket;
        std::stack<char> closed_bracket;

        for (char c : s){
            if (c == '(' or c == '{' or c == '['){
                opened_bracket.push(c);
            }

            else if (c == ')'){
                if (opened_bracket.top() == '('){
                    opened_bracket.pop();
                }
                else {
                    // invalid bracket here
                    closed_bracket.push(c);
                }
            }

            else if (c == '}'){
                if (opened_bracket.top() == '{'){
                    opened_bracket.pop();
                }
                else {
                    // invalid bracket here
                    closed_bracket.push(c);
                }
            }

            else if (c == ']'){
                if (opened_bracket.top() == '['){
                    opened_bracket.pop();
                }
                else {
                    // invalid bracket here
                    closed_bracket.push(c);
                }
            }
        }

        if (!opened_bracket.empty() or !closed_bracket.empty()){
            return false;
        }
        else return true;
    }

};

void PrintVector(std::vector<int> s){
    for (int v : s){
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::string str = "a+b*(c-d)/a";
    std::string out_str;
    std::vector<int> v = {1,5,3,6,8,4};
    std::vector<int> out_v;
    StackOperation s;

    out_str = s.ParsedAlo(str);
    std::cout << "ParsedAlo: out_str = " << out_str << std::endl;

    out_v = s.Sorted(v);
    std::cout << "Sorted: out_v: ";   
    PrintVector(out_v);

    bool res = s.CheckValidBracketString("()[{}]{([])}");
    std::cout << "CheckValidBracketString: res = " << res << std::endl;
}
