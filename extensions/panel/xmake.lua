includes("../../xmake.utils.lua")
includes("../../xmake.libs.lua")

setup()

add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", {system = true})

target("panel")
    set_extension("panel")
    add_files("*.cpp", "*.cppm")
    add_files("../theme/color.cppm")
    add_files("../theme/material.cppm")
    add_files("../theme/generate.cppm")
    add_packages("libpipewire-0.3")
    add_deps("system-ui", "material-color-utilities")