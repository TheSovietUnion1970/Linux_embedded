#include <iostream>
using namespace std;

class Outer
{
  public:
    // Nested class inside Outer
    class Inner
    {
      public:
        void display()
        {
            cout << "This is the Inner class" << endl;
        }
    };

    void show()
    {
        cout << "This is the Outer class" << endl;
    }
};

int main()
{
    // Creating object of Outer class
    Outer outerObj;
    outerObj.show();

    // Creating object of Inner class using Outer
    Outer::Inner innerObj;
    innerObj.display();

    return 0;
}