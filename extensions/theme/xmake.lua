includes("../../xmake.utils.lua")

setup()

add_requires("cairo", "libwebp", "libjpeg", "librsvg-2.0", {system = true})

local materialColorUtilitiesDir = "../../libs/material-color-utilities"
target("material-color-utilities")
    set_default(false)
    set_kind("object")
    add_files(materialColorUtilitiesDir .. "/cpp/utils/utils.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/cam/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/quantize/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/score/**.cc")
    remove_files(materialColorUtilitiesDir .. "/cpp/**_test.cc" )
    add_includedirs(materialColorUtilitiesDir, {public = true})
    
    -- Clang warnings
    -- add_cxxflags("-Wno-absolute-value")

    before_build(function (target)
        local changes = os.iorun("git status --porcelain " .. materialColorUtilitiesDir)
        if (#changes == 0) then
            os.exec("git apply libs/material-color-utilities.patch --directory=" .. materialColorUtilitiesDir)
        end
    end)

target("theme")
    set_kind("binary")
    set_build_dir()
    
    add_files("*.cpp")
    add_files("../../src/utils/log.cpp")
    
    add_deps("material-color-utilities")
    
    add_packages("cairo", "libwebp", "libjpeg", "librsvg-2.0")