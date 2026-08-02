#include "register/register.h"

#include "core/functionality/MediaHelper.h"
#include "core/math/CustomMath.h"
#include "gdextension_interface.h"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"

#include "video/VideoDecoder.h"
#include "audio/AudioDecoder.h"

#include "video/VideoRenderer.h"
#include "audio/AudioRenderer.h"

#include "video/VideoEditor.h"
#include "audio/AudioMixer.h"
#include "core/math/ColorScopeMath.h"

#include "audio/CustomAudioStreamPlayer.h"


using namespace godot;

void initialize(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    
    GDREGISTER_CLASS(ColorScopeMath);
    GDREGISTER_CLASS(CustomMath);
    
    GDREGISTER_CLASS(MediaHelper);

    GDREGISTER_CLASS(VideoDecoder);
    GDREGISTER_CLASS(VideoRenderer);
    GDREGISTER_CLASS(VideoEditor);

    GDREGISTER_CLASS(AudioDecoder);
    GDREGISTER_CLASS(AudioRenderer);
    GDREGISTER_CLASS(AudioMixer);
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
