#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

namespace godot {

class ColorScopeMath: public Resource {
    GDCLASS(ColorScopeMath, Resource);

protected:
    static void _bind_methods();

public:
    ColorScopeMath();
    ~ColorScopeMath();

    static Dictionary calculate(Ref<Image> image, PackedByteArray curr_image_data, int samples_down_scale);
};

}
