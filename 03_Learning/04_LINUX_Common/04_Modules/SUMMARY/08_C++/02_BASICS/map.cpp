#include <iostream>
#include <unordered_map>
#include <string>

int main() {
    // 1. Declaration
    std::unordered_map<std::string, int> ageMap;

    // 2. Insertion
    ageMap["Alice"] = 28;
    ageMap.insert({"Bob", 32});

    // 3. Accessing Values
    std::cout << "Alice's age: " << ageMap.at("Alice") << "\n";

    // 4. Searching
    if (ageMap.find("Charlie") == ageMap.end()) {
        std::cout << "Charlie not found.\n";
    }

    // 5. Deletion
    ageMap.erase("Bob");

    return 0;
}
