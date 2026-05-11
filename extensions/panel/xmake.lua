includes("../../xmake.utils.lua")
includes("../../xmake.libs.lua")

setup()

target("panel")
    set_extension("panel")
    add_files("*.cpp", "*.cppm")
    add_files("../theme/color.cppm")
    add_files("../theme/material.cppm")
    add_files("../theme/generate.cppm")
    add_packages("libpipewire-0.3")
    add_deps("system-ui", "material-color-utilities")