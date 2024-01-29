set_languages("c++2b")
add_rules("mode.debug", "mode.release")
if is_mode("debug") then add_defines("DEV") end

add_requires("gtk+-3.0", {system = true})
add_requires("gtk-layer-shell-0", {system = true})
add_requires("libpipewire-0.3", {system = true})

target("glaze")
    set_kind("object")
    add_includedirs("libs/glaze/include", {public = true})

target("material-color-utilities")
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
    add_rules("utils.install.pkgconfig_importfiles")
    local headerDir = "include/system-ui"
    add_installfiles("src/*.h", { prefixdir = headerDir })
    add_installfiles("src/components/*.h", {prefixdir = headerDir .. "/components"})
    after_uninstall(function (target)
        os.rm(target:installdir() .. "/" .. headerDir)
        os.rm(target:installdir() .. "/lib/pkgconfig/system-ui.pc")
    end)

target("app")
    set_basename("system-ui")
    add_deps("system-ui")
    -- LD_LIBRARY_PATH
    add_rpathdirs("@loader_path")