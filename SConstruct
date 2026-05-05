import os

env = SConscript("godot-cpp/SConstruct")

target_platform = str(env.get("platform", ""))
target_arch = str(env.get("arch", ""))
is_mingw = env.get("use_mingw", False)

if target_platform == "windows" and target_arch == "arm64" and is_mingw:
    env['ENV']['PATH'] = os.environ['PATH']
    env['CC']     = 'aarch64-w64-mingw32-gcc'
    env['CXX']    = 'aarch64-w64-mingw32-g++'
    env['LINK']   = 'aarch64-w64-mingw32-g++'
    env['AR']     = 'aarch64-w64-mingw32-ar'
    env['RANLIB'] = 'aarch64-w64-mingw32-ranlib'
    env['AS']     = 'aarch64-w64-mingw32-as'

if target_platform in ["linux", "x11"] and target_arch == "arm64":
    env['CC']     = 'aarch64-linux-gnu-gcc'
    env['CXX']    = 'aarch64-linux-gnu-g++'
    env['LINK']   = 'aarch64-linux-gnu-g++'
    env['AR']     = 'aarch64-linux-gnu-ar'
    env['RANLIB'] = 'aarch64-linux-gnu-ranlib'
    env.Append(CCFLAGS=['-march=armv8-a'])

env.Append(CPPPATH=["src/"])
sources = env.Glob("src/*.cpp")

platform_map = {"windows": "win", "linux": "linux", "x11": "linux"}
arch_map = {"x86_64": "x8664", "x86_32": "x8632", "arm64": "arm64"}

p_prefix = platform_map.get(target_platform, target_platform)
a_suffix = arch_map.get(target_arch, target_arch)
folder_name = "{}_{}".format(p_prefix, a_suffix)

ffmpeg_base = os.path.abspath("ffmpeg/bin")
ffmpeg_lib_path = os.path.join(ffmpeg_base, "lib", folder_name)
ffmpeg_include_path = os.path.join(ffmpeg_base, "include")

env.Append(CPPPATH=[ffmpeg_include_path])
env.Append(LIBPATH=[ffmpeg_lib_path])

ffmpeg_libs = ["avformat", "avcodec", "avutil", "swscale", "swresample"]

if target_platform == "windows" and not is_mingw:
    env.Append(LIBS=[lib + ".lib" for lib in ffmpeg_libs])
else:
    env.Append(LIBS=ffmpeg_libs)

if target_platform in ["linux", "x11"]:
    env.Append(LIBS=["m", "z", "dl", "pthread"])

out_dir_prefix = "win" if target_platform == "windows" else "linux"
out_dir_suffix = "arm64" if target_arch == "arm64" else ("64" if "64" in target_arch else "32")

output_path = "demo/addons/ffmpeg_codec/{}{}/VideoCodec{}{}".format(
    out_dir_prefix, 
    out_dir_suffix, 
    env["suffix"], 
    env["SHLIBSUFFIX"]
)

library = env.SharedLibrary(output_path, source=sources)

print("\n--- Build Configuration ---")
print("Target Arch:   {}".format(target_arch))
print("Using MinGW:   {}".format(is_mingw))
print("Compiler CXX:  {}".format(env['CXX']))
print("Output Path:   {}".format(output_path))
print("----------------------------------\n")

Default(library)