#ifndef AUDIO_DECODER
#define AUDIO_DECODER

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
    #include <libswresample/swresample.h>
}

namespace godot {

    class AudioDecoder: public Resource {
        GDCLASS(AudioDecoder, Resource);
        
        private:

        protected:
            static void _bind_methods();

        public:
            static TypedArray<PackedByteArray> create_data_from_path(const String &file_path);
            
            AudioDecoder();
            ~AudioDecoder();
    };

}

#endif