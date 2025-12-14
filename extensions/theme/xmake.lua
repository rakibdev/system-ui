includes("../../xmake.utils.lua")
includes("../../xmake.libs.lua")

setup()

add_requires("cairo", "libwebp", "libjpeg", "librsvg-2.0", {system = true})

target("theme")
    set_kind("binary")
    set_build_dir()
    
    add_files("*.cpp")
    add_files("../../src/utils/log.cpp")
    add_files("../../src/utils/image.cpp")
    
    add_deps("material-color-utilities")
    
    add_packages("cairo", "libwebp", "libjpeg", "librsvg-2.0")