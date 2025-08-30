includes("../../xmake.utils.lua")

setup()

add_requires("gtk+-3.0", "gtk-layer-shell-0", "glaze", {system = true})

target("panel")
    set_extension()
    add_files("*.cpp")
