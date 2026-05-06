includes("xmake.utils.lua")

setup()

local materialColorUtilitiesDir = "libs/material-color-utilities"

target("material-color-utilities")
    set_kind("static")
    set_targetdir("build")
    add_files(materialColorUtilitiesDir .. "/cpp/utils/utils.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/cam/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/quantize/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/score/**.cc")
    remove_files(materialColorUtilitiesDir .. "/cpp/**_test.cc")
    add_includedirs(materialColorUtilitiesDir, {public = true})

    add_cxxflags("-fPIC")
