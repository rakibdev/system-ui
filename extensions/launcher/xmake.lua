includes("../../xmake.utils.lua")

setup()

target("launcher")
    set_extension("launcher")
    add_files("*.cpp", "*.cppm")