includes("../../xmake.utils.lua")

setup()

add_requires("glib", {system = true})

target("media")
    set_kind("binary")
    set_build_dir()
    
    add_files("*.cpp")
    add_files("../../src/utils/log.cpp")
    add_files("../../src/services/media.cpp")
    
    add_packages("glib")
