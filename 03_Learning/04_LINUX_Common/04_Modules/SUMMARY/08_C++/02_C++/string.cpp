#include <iostream>
#include <string>
using namespace std;

int main() {
    
    // Creating a string
    string str = "Hello Geeks";

    // Traversing using index
    cout << "Using index: ";
    for (int i = 0; i < str.size(); i++) {
        cout << str[i] ;
    }
    cout << endl;

    // Traversing using range-based for loop
    cout << "Using range-based for loop: ";
    for (auto ch : str) {
        cout << ch ;
    }
    cout << endl;

    // Traversing using iterator
    cout << "Using iterator: ";
    for (auto it = str.begin(); it != str.end(); it++) {
        cout << *it ;
    }
    cout << endl;

    return 0;
}