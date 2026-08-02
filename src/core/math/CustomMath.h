#pragma once
#include "godot_cpp/classes/resource.hpp"
#include <godot_cpp/classes/object.hpp>

namespace godot {

class CustomMath : public Resource {
	GDCLASS(CustomMath, Resource)

public:
	static PackedInt32Array get_divisors(int n);
	static int find_larger_divisor(int n);

protected:
	static void _bind_methods();
};

}
