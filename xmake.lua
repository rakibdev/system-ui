add_rules("mode.debug", "mode.release")
if is_mode("debug") then
    add_defines("DEV")
    set_installdir("install")
else
    set_installdir("/usr/")
end
set_languages("c++2b")
add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3",  {system = true})

local pcFile = "/lib/pkgconfig/system-ui.pc"
local headerDir = "include/system-ui"
local shareDir = "share/system-ui"

function createPkgConfig(target)
    local file = io.open(target:installdir() .. pcFile, 'w')
    if not file then return end
    local requires = table.concat(target:get("packages"), ", ")
    local content = string.format([[
prefix=%s
libdir=${prefix}/lib
includedir=${prefix}/include

Name: system-ui
Description: UI for Linux
Version: 0.0.1
Requires: %s
Libs: -L${libdir} -lsystem-ui
Cflags: -I${includedir}/system-ui]], target:installdir(), requires)
    file:write(content)
    file:close()
end

target("glaze")
    set_default(false)
    set_kind("object")
    add_includedirs("libs/glaze/include", {public = true})

local materialColorUtilitiesDir = "libs/material-color-utilities"
target("material-color-utilities")
    set_default(false)
    set_kind("object")
    add_files(materialColorUtilitiesDir .. "/cpp/utils/utils.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/cam/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/quantize/**.cc")
    add_files(materialColorUtilitiesDir .. "/cpp/score/**.cc")
    remove_files(materialColorUtilitiesDir .. "/cpp/**_test.cc" )
    add_includedirs(materialColorUtilitiesDir, {public = true})
    
    -- fixes linking relocation error for shared "system-ui" target.
    add_cxxflags("-fPIC")

    before_build(function (target)
        local changes = os.iorun("git status --porcelain " .. materialColorUtilitiesDir)
        if (#changes == 0) then
            os.exec("git apply libs/material-color-utilities.patch --directory=" .. materialColorUtilitiesDir)
        end
    end)

target("system-ui")
    set_kind("shared")
    add_files("src/**.cpp")
    add_files("extensions/**.cpp")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3")
    add_deps("glaze")
    add_deps("material-color-utilities")
    
    -- fixes linking relocation error for .so extensions.
    add_cxxflags("-fPIC")

    -- install
    add_installfiles("src/*.h", { prefixdir = headerDir })
    add_installfiles("src/components/*.h", {prefixdir = headerDir .. "/components"})
    add_installfiles("assets/shaders/*.frag", { prefixdir = shareDir .. "/shaders" })
    add_installfiles("assets/system-ui.css", { prefixdir = shareDir })
    after_install(createPkgConfig)
    after_uninstall(function (target)
        os.rm(target:installdir() .. "/" .. headerDir)
        os.rm(target:installdir() .. pcFile)
    end)

target("app")
    set_basename("system-ui")
    add_deps("system-ui")
    -- LD_LIBRARY_PATH
    add_rpathdirs("@loader_path")