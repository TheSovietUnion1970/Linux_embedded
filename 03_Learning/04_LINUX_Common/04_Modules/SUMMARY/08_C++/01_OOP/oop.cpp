#include <iostream>
#include <string>
using namespace std;

class Employee {

    // Instance variables
private:
    string name;
    float salary;

public:
    // Constructor
    Employee(string name, float salary) {
        this->name = name;
        this->salary = salary;
    }

    // getters method
    string getName() { return name; }
    float getSalary() { return salary; }

    // setters method
    void setName(string name) {
        this->name = name;
        cout << "Setting name" << endl;
    }
    void setSalary(float salary) {
        this->salary = salary;
        cout << "Setting salary" << endl;
    }

    // Instance method
    void displayDetails() {
        cout << "Employee: " << name << endl;
        cout << "Salary: " << salary << endl;
    }
};


int main() {

    Employee emp("Geek", 10000.0f);

    // Employee emp;
    // emp.name = "Haha";
    // emp.salary = 1000;

    emp.displayDetails();
    return 0;
}