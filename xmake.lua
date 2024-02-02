set_languages("c++2b")
add_rules("mode.debug", "mode.release")
if is_mode("debug") then add_defines("DEV") end
if is_mode("release") then set_installdir("/usr/") end
add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3",  {system = true})

local pcFile = "/lib/pkgconfig/system-ui.pc"
local headerDir = "include/system-ui"

function createPkgConfig(target)
    local file = io.open(target:installdir() .. pcFile, 'w')
    if not file then return end

    local packages = table.concat(target:get("packages"), ", ")
    local content = string.format([[
prefix=%s
libdir=${prefix}/lib
includedir=${prefix}/include

Name: system-ui
Description: UI for Linux
Version: 0.0.1
Requires: %s
Libs: -L${libdir} -lsystem-ui
Cflags: -I${includedir}/system-ui]], target:installdir(), packages)
    file:write(content)
    file:close()
end

target("glaze")
    set_default(false)
    set_kind("object")
    add_includedirs("libs/glaze/include", {public = true})

target("material-color-utilities")
    set_default(false)
    set_kind("object")
    add_files("libs/material-color-utilities/cpp/**.cc")
    add_includedirs("libs/material-color-utilities", {public = true})
    
    -- fixes linking relocation error for shared "system-ui" target.
    add_cxxflags("-fPIC")

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