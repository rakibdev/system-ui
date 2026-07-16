includes("../../xmake.utils.lua")

target("theme")
    set_kind("binary")
    build_in_root()

    add_files("main.cpp", "*.cppm")
    add_files(path.join(os.projectdir(), "libs/material-colors/*.cppm"))
    add_files(sdk("utils/argparser.cppm"), sdk("utils/log.cppm"), sdk("utils/image.cppm"), sdk("utils/quantize.cppm"))

    add_packages("cairo", "libwebp", "libjpeg", "librsvg-2.0")
