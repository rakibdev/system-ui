includes("../../xmake.utils.lua")

setup()

add_requires("glib", {system = true})

target("media")
    set_kind("binary")
    set_build_dir()

    add_files("*.cpp")
    add_deps("system-ui")
    add_files("../../src/**.cppm")
    set_policy("build.c++.modules.reuse", true)

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0", "libpipewire-0.3", "glib")
