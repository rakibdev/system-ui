set_languages("c++latest")
set_defaultmode("debug")
add_rules("mode.debug", "mode.release")
if is_mode("debug") then add_defines("DEV") end

add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", {system = true})

set_installdir("/usr/")
local pcFile = "/lib/pkgconfig/system-ui.pc"
local headerDir = "include/system-ui"
local shareDir = "share/system-ui"

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
    
    -- Fixes linking relocation for shared "system-ui" target.
    add_cxxflags("-fPIC")

    before_build(function (target)
        local changes = os.iorun("git status --porcelain " .. materialColorUtilitiesDir)
        if (#changes == 0) then
            os.exec("git apply libs/material-color-utilities.patch --directory=" .. materialColorUtilitiesDir)
        end
    end)

function createPkgConfig(target)
    -- pkg-config file.
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

function removePkgConfig(target)
    os.rm(target:installdir() .. "/" .. headerDir)
    os.rm(target:installdir() .. pcFile)

target("system-ui")
    set_kind("shared")
    add_files("src/**.cpp")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze")
    add_deps("material-color-utilities")
    
    -- Fixes linking relocation for .so extensions.
    add_cxxflags("-fPIC")

    add_installfiles("src/*.h", { prefixdir = headerDir })
    add_installfiles("src/components/*.h", {prefixdir = headerDir .. "/components"})
    add_installfiles("src/default.css", { prefixdir = shareDir })
    add_installfiles("extensions", { prefixdir = shareDir .. "/extensions" })
    after_install(function (target)
        createPkgConfig(target)
    end)
    after_uninstall(function (target)
        removePkgConfig(target)
    end)

target("app")
    set_basename("system-ui")
    add_deps("system-ui")
    add_rpathdirs("@loader_path")