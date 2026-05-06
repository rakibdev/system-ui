includes("../../xmake.utils.lua")
includes("../../xmake.libs.lua")

setup()

add_requires("cairo", "libwebp", "libjpeg", "librsvg-2.0", {system = true})

target("theme")
    set_kind("binary")
    set_build_dir()

    add_files("*.cpp", "*.cppm")
    add_files("../../src/**.cppm")
    set_policy("build.c++.modules.reuse", true)
    add_deps("system-ui", "material-color-utilities")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0", "libpipewire-0.3")