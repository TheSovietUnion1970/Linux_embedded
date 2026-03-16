#include <iostream>
#include <string>
using namespace std;

// Abstract base class as there is a
// pure virtual method
class Shape{
public:
    string color;

public:
    Shape(string color) : color(color){}

    // Abstract or Pure virtual method
    virtual double area() = 0;
    virtual ~Shape() {}

    // Concrete method
    string getColor(){
        return color;
    }
};

// Derived class: Rectangle
class Rectangle : public Shape {
    double length, width;

public:
    Rectangle(string color, double length, double width)
    : Shape(color),           // ← this is the constructor of base class
      length(length),
      width(width)
    {
        // nothing here
    }

    double area() override {
        return length * width;
    }
};

int main() {
    // HEAP
    Shape* s = new Rectangle("Yellow", 2, 4);
    cout<<"Rectangle color is "<<s->getColor()<<" and area is : "<<s->area()<<endl;

    // STACK
    Shape*s1;
    Rectangle obj("Red", 2, 4);
    s1 = &obj;
    cout<<"Rectangle color is "<<s1->getColor()<<" and area is : "<<s1->area()<<endl;

    delete s;

    return 0;
}