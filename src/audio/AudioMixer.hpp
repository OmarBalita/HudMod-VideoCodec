#ifndef AUDIO_MIXER_HPP
#define AUDIO_MIXER_HPP

#include <godot_cpp/classes/resource.hpp>

namespace godot {

    class AudioMixer : public Resource {
        GDCLASS(AudioMixer, Resource);

    protected:
        static void _bind_methods();

    public:
        AudioMixer();
        ~AudioMixer();

        static PackedByteArray mix_buffers(Array buffers, float master_gain = 1.0f);

        static void packedvector2array_apply_effects(PackedVector2Array lr_data, Array effects, float volume = 1.0f, float pan = .0f);
        static void packedbytearray_apply_effects(PackedByteArray data, Array effects, float volume = 1.0f, float pan = .0f);
        static inline Vector2 frame_apply_effects(Vector2 frame, Array effects, float volume, float pan);
        static inline float sample_apply_effects(float sample, Array effects, float volume, float pan);
        static inline float sample_soft_clip(float sample);

    };

}

#endif