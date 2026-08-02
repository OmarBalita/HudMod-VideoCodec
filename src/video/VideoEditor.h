#pragma once
#include <godot_cpp/classes/resource.hpp>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

namespace godot {

class VideoEditor: public Resource {
    GDCLASS(VideoEditor, Resource);

protected:
    static void _bind_methods();

public:
    VideoEditor();
    ~VideoEditor();

    static bool resize_video_file(const String &input_path, const String &output_path, int target_width, int target_height);
};

}
