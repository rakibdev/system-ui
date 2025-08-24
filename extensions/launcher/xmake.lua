includes("../../xmake.utils.lua")

setup()

add_requires("gtk+-3.0", "gtk-layer-shell-0", "glaze", {system = true})

target("launcher")
    set_extension()
    set_build_dir()
    add_files("*.cpp")