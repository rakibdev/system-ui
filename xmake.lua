includes("xmake.utils.lua")

setup()

add_requires("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0", {system = true})

set_installdir("/usr/")
local pcFile = "/lib/pkgconfig/system-ui.pc"
local headerDir = "include/system-ui"
local shareDir = "share/system-ui"

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
end

target("system-ui")
    set_kind("shared")
    set_build_dir()

    add_files("src/**.cpp")
    -- todo: remove
    remove_files("src/services/audio.cpp")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "libpipewire-0.3", "glaze", "cairo", "libwebp", "libjpeg", "librsvg-2.0")
    
    add_cxxflags("-fPIC")

    add_installfiles("src/*.h", { prefixdir = headerDir })
    add_installfiles("src/services/*.h", {prefixdir = headerDir .. "/services"})
    add_installfiles("src/utils/*.h", {prefixdir = headerDir .. "/utils"})
    add_installfiles("src/default.css", { prefixdir = shareDir })
    add_installfiles("extensions", { prefixdir = shareDir .. "/extensions" })
    after_install(function (target)
        createPkgConfig(target)
    end)
    after_uninstall(function (target)
        removePkgConfig(target)
    end)

target("ui")
    set_build_dir()
    add_deps("system-ui")
    add_rpathdirs("@loader_path")