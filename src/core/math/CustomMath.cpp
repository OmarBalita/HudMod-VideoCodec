#include "core/math/CustomMath.h"
#include <godot_cpp/variant/packed_int32_array.hpp>

using namespace godot;

void CustomMath::_bind_methods() {
	ClassDB::bind_static_method("CustomMath", D_METHOD("get_divisors", "n"), &CustomMath::get_divisors);
	ClassDB::bind_static_method("CustomMath", D_METHOD("find_larger_divisor", "n"), &CustomMath::find_larger_divisor);
}

PackedInt32Array CustomMath::get_divisors(int n) {
	PackedInt32Array divisors;
	for (int idx = 2; idx < n; idx++) {
		if (n % idx == 0) {
			divisors.push_back(n / idx);
		}
	}
	return divisors;
}

int CustomMath::find_larger_divisor(int n) {
	PackedInt32Array divisors = get_divisors(n);
	int max_val = divisors[0];
	for (int i = 1; i < divisors.size(); i++) {
		if (divisors[i] > max_val) max_val = divisors[i];
	}
	return max_val;
}
