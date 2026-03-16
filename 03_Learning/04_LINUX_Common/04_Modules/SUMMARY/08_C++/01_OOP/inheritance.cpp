#include <iostream>
using namespace std;

/* =================================== INHERIT FROM MULTIPLE BASES */
class LandVehicle
{
  public:
    LandVehicle(){
        cout << "LandVehicle constructor" << endl;
    }
    void landInfo()
    {
        cout << "This is a LandVehicle" << endl;
    }
};

class WaterVehicle
{
  public:
    WaterVehicle(){
        cout << "WaterVehicle constructor" << endl;
    }
    void waterInfo()
    {
        cout << "This is a WaterVehicle" << endl;
    }
};

// Derived class inheriting from both base classes
class AmphibiousVehicle : public LandVehicle, public WaterVehicle
{
  public:
    AmphibiousVehicle()
    {
        cout << "AmphibiousVehicle constructor" << endl;
    }
};

/* =================================== INHERIT FROM RESPECTIVE BASES */
class Vehicle
{
  public:
    Vehicle()
    {
        cout << "This is a Vehicle" << endl;
    }
};

// Derived class from Vehicle
class FourWheeler : public Vehicle
{
  public:
    FourWheeler()
    {
        cout << "4 Wheeler Vehicles" << endl;
    }
};

// Derived class from FourWheeler
class Car : public FourWheeler
{
  public:
    Car()
    {
        cout << "This 4 Wheeler Vehicle is a Car" << endl;
    }
};

int main()
{
    /* =================================== INHERIT FROM MULTIPLE BASES */
    AmphibiousVehicle obj;

    obj.waterInfo();
    obj.landInfo();

    /* =================================== INHERIT FROM RESPECTIVE BASES */
    Car obj1;

    return 0;
}