#include <iostream>
#include <string>
using namespace std;

// Class representing a Programmer
class Programmer
{
  private:
    string name; // Private variable => Encapsulate here

  public:
    // Getter method to access the private data
    string getName()
    {
        return name;
    }

    // Setter method to modify the private data
    void setName(string newName)
    {
        name = newName;
    }
};

int main()
{
    Programmer p;
    p.setName("Geek");                        // Set the name
    cout << "Name=> " << p.getName() << endl; // Get the name
    return 0;
}