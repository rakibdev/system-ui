function setup()
    set_languages("c++26")
    set_toolchains("clang")
    set_defaultmode("debug")
    add_rules("mode.debug", "mode.release")
    add_cxxflags("-Wno-absolute-value")

    if is_mode("debug") then
        add_defines("DEV")
        add_defines('SHARE_DIR="' .. os.projectdir() .. '"')
    else
        add_defines('SHARE_DIR="/usr/share/system-ui"')
    end
end

function set_build_dir()
    set_targetdir("build")
end

function set_extension(extName)
    set_kind("shared")
    set_targetdir("build")

    -- Fixes linking relocation for .so extensions.
    add_cxxflags("-fPIC")

    add_linkdirs("../../build/")
    add_links("system-ui")

    -- Remove "lib" prefix
    set_prefixname("")

    add_packages("gtk+-3.0", "gtk-layer-shell-0", "glaze")

    if is_mode("debug") then
        add_defines('EXT_DIR="' .. os.scriptdir() .. '"')
    else
        add_defines('EXT_DIR="/usr/share/' .. (extName or "system-ui") .. '"')
    end
end
