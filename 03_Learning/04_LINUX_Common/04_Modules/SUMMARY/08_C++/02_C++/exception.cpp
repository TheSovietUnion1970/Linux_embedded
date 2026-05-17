#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <exception>
using namespace std;

// ====================== 1. Custom Exception Class ======================
class MyException : public exception {
public:
    string message;
    MyException(string msg) : message(msg) {}
    
    const char* what() const noexcept override {
        return (message.c_str());
    }

    const char* what_custom() const {
        return (message.c_str());
    }
};

// ====================== 2. Functions that throw exceptions ======================
void divide(int a, int b) {
    if (b == 0)
        throw runtime_error("Division by zero!");   // Standard exception
    cout << "Result = " << a / b << endl;
}

void accessVector(const vector<int>& v, size_t index) {
    if (index >= v.size())
        throw out_of_range("Index out of range!");
    cout << "Value = " << v[index] << endl;
}

void testNoExcept() noexcept {      // Function promises not to throw
    cout << "This function will not throw exceptions\n";
    //throw 10;   // Error if uncommented
}

// ====================== MAIN DEMO ======================
int main() {
    cout << "=== C++ Exception Handling - All Usages ===\n\n";

    // 1. Basic try-catch
    try {
        divide(10, 0);
    } 
    catch (const runtime_error& e) {
        cout << "Caught runtime_error: " << e.what() << endl;
    }

    // 2. Multiple catch blocks
    try {
        vector<int> v = {1, 2, 3};
        accessVector(v, 10);
    } 
    catch (const out_of_range& e) {
        cout << "Caught out_of_range: " << e.what() << endl;
    } 
    catch (const exception& e) {           // Catch base class
        cout << "Caught exception: " << e.what() << endl;
    }

    // 3. Catch by value (not recommended), by reference, and const reference
    try {
        throw MyException("Custom error occurred!");
    } 
    catch (MyException e) {                    // By value (copy)
        cout << "Caught by value: " << e.what() << endl;
    }

    // 4. Best practice: catch by const reference
    try {
        throw MyException("Another custom error");
    } 
    catch (const MyException& e) {             // Best way
        cout << "Caught by const ref: " << e.what_custom() << endl;
    }

    // 5. Catch all exceptions (catch-all handler)
    try {
        throw string("Unknown error type");
    } 
    catch (...) {                              // Catch anything
        cout << "Caught unknown exception using catch(...)\n";
    }

    // 6. Re-throwing exception
    try {
        try {
            throw runtime_error("Inner error");
        } 
        catch (const exception& e) {
            cout << "Inner catch: " << e.what() << endl;
            throw;        // Re-throw the same exception
        }
    } 
    catch (const exception& e) {
        cout << "Outer catch (re-thrown): " << e.what() << endl;
    }

    // 7. noexcept demonstration
    try {
        testNoExcept();
    } 
    catch (...) {
        cout << "This will never be executed because of noexcept\n";
    }

    // 8. Using std::exception hierarchy
    try {
        throw invalid_argument("Invalid argument passed");
    } 
    catch (const invalid_argument& e) {
        cout << "invalid_argument: " << e.what() << endl;
    }

    // 9. Throwing from constructor / destructor (note)
    cout << "\nNote: Throwing from destructor is dangerous and should be avoided.\n";

    cout << "\n=== All exception usage examples completed ===\n";
    return 0;
}