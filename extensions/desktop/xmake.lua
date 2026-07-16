includes("../../xmake.utils.lua")

target("desktop")
    set_extension("desktop")
    add_files("*.cpp")
    add_files(path.join(os.projectdir(), "libs/material-colors/*.cppm"))
