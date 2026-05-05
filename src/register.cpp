#include "register.hpp"

#include "gdextension_interface.h"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"

#include "VideoDecoder.hpp"
#include "AudioDecoder.hpp"

#include "VideoRenderer.hpp"
#include "AudioRenderer.hpp"

#include "VideoEditor.hpp"
#include "AudioMixer.hpp"
#include "ColorScopeMath.hpp"

#include "CustomAudioStreamPlayer.hpp"


using namespace godot;

void initialize(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_CLASS(VideoDecoder);
    GDREGISTER_CLASS(AudioDecoder);
    GDREGISTER_CLASS(VideoRenderer);
    GDREGISTER_CLASS(AudioRenderer);
    GDREGISTER_CLASS(AudioMixer);
    GDREGISTER_CLASS(VideoEditor);
    GDREGISTER_CLASS(ColorScopeMath);
    GDREGISTER_CLASS(CustomAudioStreamPlayer);
}

void uninitialize(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_intialization) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_intialization);
        init_obj.register_initializer(initialize);
        init_obj.register_terminator(uninitialize);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
        return init_obj.init();
    };
}
