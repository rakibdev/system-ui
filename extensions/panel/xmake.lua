includes("../../xmake.utils.lua")
includes("../../xmake.libs.lua")

setup()

add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", {system = true})

target("panel")
    set_extension("panel")
    add_files("*.cpp")
    add_files("../theme/theme.cpp")
    add_files("../theme/color.cpp") 
    add_files("../theme/material.cpp")
    add_packages("libpipewire-0.3")
    add_deps("material-color-utilities")