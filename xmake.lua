includes("xmake.utils.lua")

setup()

add_requires("gtk4", "gtk4-layer-shell", "libpipewire-0.3", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0", {system = true})

target("system-ui")
    set_kind("shared")
    build_in_root()
    add_rules("c++.build.modules")
    add_files("src/**.cpp", "src/**.cppm")
    add_files("libs/material-colors/oklch.cppm", "libs/material-colors/hct-oklch.cppm")
    add_packages("gtk4", "gtk4-layer-shell", "libpipewire-0.3", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0", {public = true})
    add_cxxflags("-fPIC")
    if is_mode("debug") then
        add_defines('SHARE_DIR="' .. os.projectdir() .. '"')
    else
        add_defines('SHARE_DIR="/usr/share/system-ui"')
    end

target("ui")
    build_in_root()
    add_deps("system-ui")
    add_rpathdirs("@loader_path")

includes("extensions/launcher/xmake.lua")
includes("extensions/panel/xmake.lua")
includes("extensions/theme/xmake.lua")
includes("extensions/media/xmake.lua")
