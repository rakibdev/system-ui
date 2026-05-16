includes("../../xmake.utils.lua")
includes("../../libs/xmake.material.lua")

target("theme")
    set_kind("binary")
    build_in_root()

    add_files("main.cpp", "*.cppm")
    add_files(sdk("utils/argparser.cppm"), sdk("utils/log.cppm"), sdk("utils/image.cppm"))
    add_deps("material-color-utilities")

    add_packages("cairo", "libwebp", "libjpeg", "librsvg-2.0")
