#include <bits/stdc++.h>
#include <string>
using namespace std;

class Geeks {
public:

    // Function to add two integers
    void add(int a, int b) {
        cout << "Integer Sum = " << a + b
        << endl;
    }

    // Function to add two floating point values
    void add(double a, double b) {
        cout << "Float Sum = " << a + b
        << endl ;
    }

    void add(int a){
        cout << "Int add 5 = " << a + 5
        << endl ;
    }
};

// Functions/Operators can be overloaded either by changing the number of arguments or changing the type of arguments. - during compile time

class Complex {
public:
    int real, imag;

    Complex(int r, int i)
    : real(r), imag(i)
    {
    }

    // Overloading the '+' operator
    Complex operator+(const Complex& obj) {
        return Complex(real + obj.real,
        imag + obj.imag);
    }

    const Complex operator- (const Complex& ins){
        return Complex(real - ins.real, imag - ins.imag);
    }
};

class IntStr{
public:
    int a; std::string b;

    IntStr(int a, std::string b)
    : a(a), b(b) {}

    const IntStr operator+ (const IntStr& obj){
        return IntStr(a + obj.a, b + obj.b);
    }
};

int main() {
    // ========== function overloaded
    cout << "Function overloaded" << endl;
    Geeks gfg;

    // add() called with int values
    gfg.add(10, 2);

    // add() called with double value
    gfg.add(5.3, 6.2);

    gfg.add(10);

    // ========== operator num overloaded
    cout << "Operator num overloadedd" << endl;
    Complex c1(10, 5), c2(2, 4);

    // Adding c1 and c2 using + operator
    Complex c3 = c1 + c2;
    cout << "Final " << c3.real << " + i" << c3.imag << endl;

    Complex c4 = c1 - c2;
    cout << "Final " << c4.real << " + i" << c4.imag << endl;

    // ========== operator string overloaded
    cout << "Operator string overloadedd" << endl;
    IntStr s1(1, "One"), s2(2, "two");

    IntStr s3 = s1 + s2;
    cout << "Final " << s3.a << " - " << s3.b << endl;
    return 0;
}