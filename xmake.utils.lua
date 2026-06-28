function setup()
    set_languages("c++26")
    set_toolchains("clang")
    set_defaultmode("debug")
    add_rules("mode.debug", "mode.release")
    add_cxxflags("-Wno-absolute-value")
    add_rules("c++.build.modules")

    if is_mode("debug") then
        add_defines("DEV")
    end
end

function build_in_root()
    set_targetdir(os.projectdir() .. "/build")
end

function sdk(path)
    return os.projectdir() .. "/src/" .. path
end

function set_extension(extName)
    set_kind("shared")
    set_targetdir(os.projectdir() .. "/build")

    -- Fixes linking relocation for .so extensions.
    add_cxxflags("-fPIC")

    add_linkdirs(os.projectdir() .. "/build")
    add_links("system-ui")

    -- Remove "lib" prefix
    set_prefixname("")

    add_packages("gtk4", "gtk4-layer-shell", "glaze")

    -- Reuse system-ui module BMIs instead of recompiling
    set_policy("build.c++.modules.reuse", true)
    add_files("../../src/**.cppm")
    add_files(path.join(os.projectdir(), "libs/material-colors/oklch.cppm"),
              path.join(os.projectdir(), "libs/material-colors/hct-oklch.cppm"))
    add_packages("cairo", "libwebp", "libjpeg", "librsvg-2.0", "libpipewire-0.3")

    add_defines('EXT_DIR="' .. os.scriptdir() .. '"')
end
