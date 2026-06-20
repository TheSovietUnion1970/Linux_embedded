#include <iostream>
#include <string>
#include <stack>
using namespace std;

void CheckValidString(string A){
    stack<int> opened_round;
    stack<int> opened_square;
    stack<int> opened_curly;

    stack<int> closed_round;
    stack<int> closed_square;
    stack<int> closed_curly;

    bool IsInvalid = false;

    for (int i = 0; i < A.length(); i++){
        if (A.at(i) == '(') opened_round.push(i);
        else if (A.at(i) == '[') opened_square.push(i);
        else if (A.at(i) == '{') opened_curly.push(i);

        if (A.at(i) == ')'){
            if (!opened_round.empty()){
                opened_round.pop();
            }
            else {
                closed_round.push(i);
            }
        }

        else if (A.at(i) == ']'){
            if (!opened_square.empty()){
                opened_square.pop();
            }
            else {
                closed_square.push(i);
            }
        }

        else if (A.at(i) == '}'){
            if (!opened_curly.empty()){
                opened_curly.pop();
            }
            else {
                closed_curly.push(i);
            }
        }
    }

    while(!opened_round.empty()) {
        cout << "Invalid opened round: " << opened_round.top() << endl;
        opened_round.pop();
        IsInvalid = true;
    }
    while(!closed_round.empty()) {
        cout << "Invalid closed round: " << closed_round.top() << endl;
        closed_round.pop();
        IsInvalid = true;
    }

    while(!opened_square.empty()) {
        cout << "Invalid opened square: " << opened_square.top() << endl;
        opened_square.pop();
        IsInvalid = true;
    }
    while(!closed_square.empty()) {
        cout << "Invalid closed square: " << closed_square.top() << endl;
        closed_square.pop();
        IsInvalid = true;
    }

    while(!opened_curly.empty()) {
        cout << "Invalid opened curly: " << opened_curly.top() << endl;
        opened_curly.pop();
        IsInvalid = true;
    }
    while(!closed_curly.empty()) {
        cout << "Invalid closed curly: " << closed_curly.top() << endl;
        closed_curly.pop();
        IsInvalid = true;
    }

    if (!IsInvalid) cout << "Valid string" << endl;

}

int main() {
    // string A = "([)]";
    string A = "(([]";
    CheckValidString(A);
    return 0;
}