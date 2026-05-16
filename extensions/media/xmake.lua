includes("../../xmake.utils.lua")

add_requires("gio-2.0", {system = true})

target("media")
    set_kind("binary")
    build_in_root()

    add_files("*.cpp")
    add_files(sdk("services/media.cppm"), sdk("utils/log.cppm"), sdk("utils/argparser.cppm"))

    add_packages("gio-2.0")
