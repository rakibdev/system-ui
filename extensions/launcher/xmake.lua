includes("../../xmake.utils.lua")

target("launcher")
    set_extension("launcher")
    add_files("*.cpp", "*.cppm")