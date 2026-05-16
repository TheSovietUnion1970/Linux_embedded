#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
	vector<int> v = {5, 1, 8, 3, 9, 2};

	auto firstVal = [](vector<int> a){
		return *a.begin();
	};

	auto PrintType = [](){
		cout << "Lambda here\n" << endl;
	};

	auto ModifyRef = [&v](int idx, int val){
		v[idx] = val;
	};

	// or [v]
	auto ModifyVal = [=](int idx, int val) mutable {
		v[idx] = val;
	};

	auto PrintVal = [&v](int idx){
		cout << "At idx: " << idx << " -> " << v[idx] << endl;
	};

	for (int x : v)
		cout << x << " ";
    cout << endl;

	cout << firstVal(v) << endl;

	PrintType();

	ModifyRef(1, -1);
	PrintVal(1); // val is changed

	ModifyVal(2, -1);
	PrintVal(2); // val is not changed

	return 0;
}