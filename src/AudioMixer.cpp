#include "AudioMixer.hpp"

namespace godot {

    void AudioMixer::_bind_methods() {
        ClassDB::bind_static_method("AudioMixer", D_METHOD("mix_buffers", "buffers", "master_gain"), &AudioMixer::mix_buffers);
        ClassDB::bind_static_method("AudioMixer", D_METHOD("packedbytearray_apply_effects", "data", "effects", "volume", "pan"), &AudioMixer::packedbytearray_apply_effects);
        ClassDB::bind_static_method("AudioMixer", D_METHOD("packedvector2array_apply_effects", "lr_data", "effects", "volume", "pan"), &AudioMixer::packedvector2array_apply_effects);
        ClassDB::bind_static_method("AudioMixer", D_METHOD("sample_apply_effects", "sample", "effects", "volume", "pan"), &AudioMixer::sample_apply_effects);
        ClassDB::bind_static_method("AudioMixer", D_METHOD("frame_apply_effects", "frame", "effects", "volume", "pan"), &AudioMixer::frame_apply_effects);
    }

    AudioMixer::AudioMixer() {}
    AudioMixer::~AudioMixer() {}

    PackedByteArray AudioMixer::mix_buffers(Array buffers, float master_gain) {

        if (buffers.is_empty()) return PackedByteArray();
        
        PackedByteArray result = buffers[0];
        float* result_ptr = reinterpret_cast<float*>(result.ptrw());
        
        int buffers_count = buffers.size();
        int max_size = result.size() / sizeof(float);

        for (int idx = 1; idx < buffers_count; idx++) {
            const PackedByteArray &other_buffer = buffers[idx];
            const float* other_ptr = reinterpret_cast<const float*>(other_buffer.ptr());

            for (int i = 0; i < max_size; i++) {
                result_ptr[i] += other_ptr[i];
            }
        }

        for (int i = 0; i < max_size; i++) {
            float sample = result_ptr[i] * master_gain;
            result_ptr[i] = AudioMixer::sample_soft_clip(sample);
        }

        return result;
    }

    void AudioMixer::packedvector2array_apply_effects(PackedVector2Array lr_data, Array effects, float volume, float pan) {
        // Implementation for applying effects to packed vector2 array
    }

    void AudioMixer::packedbytearray_apply_effects(PackedByteArray data, Array effects, float volume, float pan) {
        // Implementation for applying effects to packed byte array
    }
    
    inline Vector2 AudioMixer::frame_apply_effects(Vector2 frame, Array effects, float volume, float pan) {
        // Implementation for applying effects to a frame (stereo sample)
        return frame;
    }

    inline float AudioMixer::sample_apply_effects(float sample, Array effects, float volume, float pan) {
        // Implementation for applying effects to a single sample
        return sample;
    }

    inline float AudioMixer::sample_soft_clip(float sample) {
        if (sample > 1.0f) return 1.0f;
        if (sample < -1.0f) return -1.0f;
        return (3.0f * sample - (sample * sample * sample)) / 2.0f;
    }

}

