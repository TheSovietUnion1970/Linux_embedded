#include <iostream>
#include <vector>
#include <algorithm>    // for sort, find, etc.
using namespace std;

void printVector(const vector<int>& v, string name = "vector") {
    cout << name << " = { ";
    for (auto x : v) cout << x << " ";
    cout << "}  size = " << v.size() << ", capacity = " << v.capacity() << endl;
}

int main() {
    // ====================== 1. Declaration & Initialization ======================
    vector<int> v1;                    // empty vector
    vector<int> v2 = {10, 20, 30, 40, 50};  // initializer list
    vector<int> v3(5, 100);            // 5 elements with value 100
    vector<int> v4(v2);                // copy constructor
    vector<int> v5{1, 2, 3, 4, 5};    // uniform initialization

    printVector(v2, "v2");

    // ====================== 2. Adding Elements ======================
    v1.push_back(5);                   // add at the end
    v1.push_back(6);
    v1.emplace_back(7);                // faster than push_back (in-place construction)
    v1.emplace_back(8);

    cout << "\nAfter push_back & emplace_back:\n";
    printVector(v1, "v1");

    // Insert at specific position
    v1.insert(v1.begin(), 99);         // insert at beginning
    v1.insert(v1.begin() + 2, 777);    // insert at index 2
    v1.insert(v1.end(), {100, 200});   // insert multiple elements

    printVector(v1, "v1 after insert");

    // ====================== 3. Accessing Elements ======================
    cout << "\nAccessing elements:\n";
    cout << "v2[0] = " << v2[0] << endl;
    cout << "v2.at(2) = " << v2.at(2) << endl;   // safer, throws exception if out of range
    cout << "front() = " << v2.front() << endl;
    cout << "back()  = " << v2.back() << endl;

    // ====================== 4. Removing Elements ======================
    v1.pop_back();                     // remove last element
    cout << "\nAfter pop_back():\n";
    printVector(v1, "v1");

    v1.erase(v1.begin());              // erase first element
    v1.erase(v1.begin() + 3);          // erase element at index 3
    v1.erase(v1.begin() + 1, v1.begin() + 4);  // erase a range

    cout << "After erase operations:\n";
    printVector(v1, "v1");

    // ====================== 5. Size & Capacity ======================
    cout << "\nSize & Capacity:\n";
    cout << "v1.size()     = " << v1.size() << endl;
    cout << "v1.capacity() = " << v1.capacity() << endl;
    cout << "v1.empty()    = " << (v1.empty() ? "true" : "false") << endl;

    v1.reserve(20);                    // pre-allocate memory
    cout << "After reserve(20), capacity = " << v1.capacity() << endl;

    v1.shrink_to_fit();                // reduce capacity to fit size
    cout << "After shrink_to_fit(), capacity = " << v1.capacity() << endl;

    // ====================== 6. Modifying Content ======================
    v2.resize(8, 999);                 // resize and fill new elements with 999
    printVector(v2, "v2 after resize");

    v2.clear();                        // remove all elements
    cout << "After clear(), size = " << v2.size() << endl;

    // ====================== 7. Algorithms with vector ======================
    vector<int> nums = {5, 2, 8, 1, 9, 3, 7};
    printVector(nums, "nums before sort");

    sort(nums.begin(), nums.end());          // sort
    printVector(nums, "nums after sort");

    auto it = find(nums.begin(), nums.end(), 8);
    if (it != nums.end())
        cout << "Found 8 at position: " << (it - nums.begin()) << endl;

    // ====================== 8. Swap & Assign ======================
    vector<int> a = {1, 2, 3};
    vector<int> b = {10, 20, 30, 40};

    a.swap(b);                         // fast swap
    printVector(a, "a after swap");
    printVector(b, "b after swap");

    a.assign({100, 200, 300});         // assign new values
    printVector(a, "a after assign");

    return 0;
}